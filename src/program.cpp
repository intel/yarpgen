/*
Copyright (c) 2015-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
     http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////////////////

#include "program.h"
#include "data.h"
#include "emit_policy.h"
#include "stmt.h"
#include <fstream>
#include <memory>
#include <sstream>

using namespace yarpgen;

ProgramGenerator::ProgramGenerator() : hash_seed(0) {
    // Generate the general structure of the test
    auto gen_ctx = std::make_shared<GenCtx>();
    new_test = ScopeStmt::generateStructure(gen_ctx);

    // Prepare to generate some math inside the structure
    ext_inp_sym_tbl = std::make_shared<SymbolTable>();
    ext_out_sym_tbl = std::make_shared<SymbolTable>();
    auto pop_ctx = std::make_shared<PopulateCtx>();
    auto gen_pol = pop_ctx->getGenPolicy();

    // Create some number of ScalarVariables that we will use to provide input
    // data to the test program
    size_t inp_vars_num = rand_val_gen->getRandValue(gen_pol->min_inp_vars_num,
                                                     gen_pol->max_inp_vars_num);
    for (size_t i = 0; i < inp_vars_num; ++i) {
        auto new_var = ScalarVar::create(pop_ctx);
        ext_inp_sym_tbl->addVar(new_var);
        ext_inp_sym_tbl->addVarExpr(
            std::make_shared<ScalarVarUseExpr>(new_var));
    }

    pop_ctx->setExtInpSymTable(ext_inp_sym_tbl);
    pop_ctx->setExtOutSymTable(ext_out_sym_tbl);

    new_test->populate(pop_ctx);

    // Create a special variable that we use to hide the information from
    // compiler
    auto zero_var = std::make_shared<ScalarVar>(
        "zero", IntegralType::init(IntTypeID::INT),
        IRValue(IntTypeID::INT, IRValue::AbsValue{false, 0}));
    zero_var->setIsDead(false);
    ext_inp_sym_tbl->addVar(zero_var);
}

void ProgramGenerator::emitCheckFunc(std::ostream &stream) {
    std::ostream &out_file = stream;
    out_file << "#include <stdio.h>\n\n";

    Options &options = Options::getInstance();
    if (options.getCheckAlgo() == CheckAlgo::ASSERTS) {
        stream << "static ";
        stream << (options.isC() ? "_Bool" : "bool") << " value_mismatch = ";
        stream << (options.isC() ? "0" : "false") << ";\n";
    }

    // The exact same function should be used for hash pre-computation!

    out_file << "unsigned long long int seed = 0;\n";
    out_file << "void hash(unsigned long long int *seed, unsigned long long "
                "int const v) {\n";
    out_file << "    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);\n";
    out_file << "}\n\n";
}

static void emitVarsDecl(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                         std::vector<std::shared_ptr<ScalarVar>> vars) {
    Options &options = Options::getInstance();
    if (options.isSYCL())
        ctx->setSYCLPrefix("app_");
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        auto init_val = std::make_shared<ConstantExpr>(var->getInitValue());
        auto decl_stmt = std::make_shared<DeclStmt>(var, init_val);
        decl_stmt->emit(ctx, stream);
        stream << "\n";
    }
    ctx->setSYCLPrefix("");
}

static void emitArrayDecl(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::vector<std::shared_ptr<Array>> arrays) {
    Options &options = Options::getInstance();
    for (auto &array : arrays) {
        if (!options.getAllowDeadData() && array->getIsDead())
            continue;
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        stream << array_type->getBaseType()->getName(ctx) << " ";
        stream << array->getName(ctx) << " ";
        for (const auto &dimension : array_type->getDimensions()) {
            stream << "[" << dimension << "] ";
        }
        if (array->getAlignment() != 0)
            stream << "__attribute__((aligned(" << array->getAlignment()
                   << ")))";
        stream << ";\n";
    }
}

void ProgramGenerator::emitDecl(std::shared_ptr<EmitCtx> ctx,
                                std::ostream &stream) {
    emitVarsDecl(ctx, stream, ext_inp_sym_tbl->getVars());
    emitVarsDecl(ctx, stream, ext_out_sym_tbl->getVars());

    emitArrayDecl(ctx, stream, ext_inp_sym_tbl->getArrays());
    emitArrayDecl(ctx, stream, ext_out_sym_tbl->getArrays());
}

static void emitArrayInit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::vector<std::shared_ptr<Array>> arrays) {
    Options &options = Options::getInstance();
    for (const auto &array : arrays) {
        if (!options.getAllowDeadData() && array->getIsDead())
            continue;
        std::string offset = "    ";
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        size_t idx = 0;
        for (const auto &dimension : array_type->getDimensions()) {
            stream << offset << "for (size_t i_" << idx << " = 0; i_" << idx
                   << " < " << dimension << "; ++i_" << idx << ") \n";
            offset += "    ";
            idx++;
        }
        stream << offset << array->getName(ctx) << " ";
        for (size_t i = 0; i < idx; ++i)
            stream << "[i_" << i << "] ";
        stream << "= ";
        auto emit_const_expr = [&array, &ctx, &stream](bool use_main_vals) {
            auto init_val = array->getInitValues(use_main_vals);
            auto init_const = std::make_shared<ConstantExpr>(init_val);
            init_const->emit(ctx, stream);
        };
        if (array->getMulValsAxisIdx() != -1) {
            stream << "(i_" << array->getMulValsAxisIdx() << " % "
                   << Options::vals_number << " == " << Options::main_val_idx
                   << ") ? ";
        }
        emit_const_expr(true);
        if (array->getMulValsAxisIdx() != -1) {
            stream << " : ";
            emit_const_expr(false);
        }
        stream << ";\n";
    }
}

void ProgramGenerator::emitInit(std::shared_ptr<EmitCtx> ctx,
                                std::ostream &stream) {
    stream << "void init() {\n";
    emitArrayInit(ctx, stream, ext_inp_sym_tbl->getArrays());
    emitArrayInit(ctx, stream, ext_out_sym_tbl->getArrays());
    stream << "}\n\n";
}

void ProgramGenerator::emitCheck(std::shared_ptr<EmitCtx> ctx,
                                 std::ostream &stream) {
    stream << "void checksum() {\n";

    Options &options = Options::getInstance();

    auto emit_pol = ctx->getEmitPolicy();

    if (options.isSYCL())
        ctx->setSYCLPrefix("app_");

    for (auto &var : ext_out_sym_tbl->getVars()) {
        std::string var_name = var->getName(ctx);

        if (options.getCheckAlgo() == CheckAlgo::HASH ||
            options.getCheckAlgo() == CheckAlgo::PRECOMPUTE) {
            stream << "    hash(&seed, " << var_name << ");\n";
            if (options.getCheckAlgo() == CheckAlgo::PRECOMPUTE)
                hash(var->getCurrentValue().getAbsValue().value);
        }
        else if (options.getCheckAlgo() == CheckAlgo::ASSERTS) {
            auto const_val =
                std::make_shared<ConstantExpr>(var->getCurrentValue());
            stream << "    value_mismatch |= " << var_name << " != ";
            const_val->emit(ctx, stream);
            stream << ";\n";
        }
        else {
            ERROR("Unsupported");
        }
    }

    ctx->setSYCLPrefix("");

    for (const auto &array : ext_out_sym_tbl->getArrays()) {
        std::string offset = "    ";
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        size_t idx = 0;
        std::stringstream ss;
        ss << array->getName(ctx) << " ";
        for (const auto &dimension : array_type->getDimensions()) {
            stream << offset << "for (size_t i_" << idx << " = 0; i_" << idx
                   << " < " << dimension << "; ++i_" << idx << ") \n";
            ss << "[i_" << idx << "] ";
            offset += "    ";
            idx++;
        }

        if (options.getCheckAlgo() == CheckAlgo::HASH ||
            options.getCheckAlgo() == CheckAlgo::PRECOMPUTE) {
            stream << offset << "hash(&seed, ";
            if (options.getCheckAlgo() == CheckAlgo::PRECOMPUTE)
                hashArray(array);
        }
        else if (options.getCheckAlgo() == CheckAlgo::ASSERTS)
            stream << offset << "value_mismatch |= ";
        else
            ERROR("Unsupported");

        std::string arr_name = ss.str();
        stream << arr_name;

        if (options.getCheckAlgo() == CheckAlgo::ASSERTS) {
            auto const_val =
                std::make_shared<ConstantExpr>((array->getCurrentValues(true)));
            stream << "!= ";
            const_val->emit(ctx, stream);
            auto emit_cmp = [&arr_name, &ctx, &stream](IRValue val) {
                stream << " && " << arr_name << "!= ";
                auto const_val = std::make_shared<ConstantExpr>(val);
                const_val->emit(ctx, stream);
            };
            emit_cmp(array->getInitValues(true));
            if (array->getMulValsAxisIdx() != -1) {
                emit_cmp(array->getCurrentValues(false));
                emit_cmp(array->getInitValues(false));
            }
        }
        else
            stream << ")";
        stream << ";\n";
    }
    stream << "}\n";
}

// This buffer tracks what input data we pass as a parameters to test functions
static std::vector<std::string> pass_as_param_buffer;
static bool any_vars_as_params = false;
static bool any_arrays_as_params = false;

static void emitVarExtDecl(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                           std::vector<std::shared_ptr<ScalarVar>> vars,
                           bool inp_category) {
    auto emit_pol = ctx->getEmitPolicy();
    Options &options = Options::getInstance();
    if (options.isSYCL())
        ctx->setSYCLPrefix("app_");
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        bool pass_as_param = false;
        if (inp_category) {
            if (options.inpAsArgs() == OptionLevel::SOME)
                pass_as_param =
                    rand_val_gen->getRandId(emit_pol->pass_as_param_distr);
            else if (options.inpAsArgs() == OptionLevel::ALL)
                pass_as_param = true;
        }

        if (pass_as_param) {
            pass_as_param_buffer.push_back(var->getName(ctx));
            any_vars_as_params = true;
            continue;
        }
        stream << "extern ";
        stream << var->getType()->getName(ctx);
        stream << " ";
        stream << var->getName(ctx) << ";\n";
    }
    ctx->setSYCLPrefix("");
}

static void emitArrayExtDecl(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                             std::vector<std::shared_ptr<Array>> arrays,
                             bool inp_category) {
    auto emit_pol = ctx->getEmitPolicy();
    Options &options = Options::getInstance();
    for (auto &array : arrays) {
        if (!options.getAllowDeadData() && array->getIsDead())
            continue;
        bool pass_as_param = false;
        if (inp_category) {
            if (options.inpAsArgs() == OptionLevel::SOME)
                pass_as_param =
                    rand_val_gen->getRandId(emit_pol->pass_as_param_distr);
            else if (options.inpAsArgs() == OptionLevel::ALL)
                pass_as_param = true;
        }

        if (pass_as_param) {
            pass_as_param_buffer.push_back(array->getName(ctx));
            any_arrays_as_params = true;
            continue;
        }

        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        stream << "extern ";
        stream << array_type->getBaseType()->getName(ctx);
        stream << " ";
        stream << array->getName(ctx) << " ";
        for (const auto &dimension : array_type->getDimensions()) {
            stream << "[" << dimension << "] ";
        }

        if (options.isCXX() &&
            options.getEmitAlignAttr() != OptionLevel::NONE) {
            bool emit_align_attr = true;
            if (options.getEmitAlignAttr() == OptionLevel::SOME)
                emit_align_attr =
                    rand_val_gen->getRandId(emit_pol->emit_align_attr_distr);
            if (emit_align_attr) {
                AlignmentSize align_size = options.getAlignSize();
                if (!options.getUniqueAlignSize())
                    align_size =
                        rand_val_gen->getRandId(emit_pol->align_size_distr);
                size_t alignment = 0;
                switch (align_size) {
                    case AlignmentSize::A16:
                        alignment = 16;
                        break;
                    case AlignmentSize::A32:
                        alignment = 32;
                        break;
                    case AlignmentSize::A64:
                        alignment = 64;
                        break;
                    case AlignmentSize::MAX_ALIGNMENT_SIZE:
                        ERROR("Bad alignment size");
                }
                array->setAlignment(alignment);
                stream << "__attribute__((aligned(" << alignment << ")))";
            }
        }

        stream << ";\n";
    }
}

void ProgramGenerator::emitExtDecl(std::shared_ptr<EmitCtx> ctx,
                                   std::ostream &stream) {
    Options &options = Options::getInstance();
    if (options.isISPC())
        ctx->setIspcTypes(true);
    emitVarExtDecl(ctx, stream, ext_inp_sym_tbl->getVars(), true);
    emitVarExtDecl(ctx, stream, ext_out_sym_tbl->getVars(), false);
    emitArrayExtDecl(ctx, stream, ext_inp_sym_tbl->getArrays(), true);
    emitArrayExtDecl(ctx, stream, ext_out_sym_tbl->getArrays(), false);
    ctx->setIspcTypes(false);
}

static std::string placeSep(bool cond) { return cond ? ", " : ""; }

static bool emitVarFuncParam(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                             std::vector<std::shared_ptr<ScalarVar>> vars,
                             bool emit_type, bool ispc_type) {
    bool emit_any = false;
    Options &options = Options::getInstance();
    if (options.isSYCL())
        ctx->setSYCLPrefix("app_");
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        if (std::find(pass_as_param_buffer.begin(), pass_as_param_buffer.end(),
                      var->getName(ctx)) == pass_as_param_buffer.end())
            continue;

        stream << placeSep(emit_any);
        if (emit_type)
            stream << var->getType()->getName(ctx) << " ";
        stream << var->getName(ctx);
        emit_any = true;
    }
    ctx->setSYCLPrefix("");
    return emit_any;
}

static void emitArrayFuncParam(std::shared_ptr<EmitCtx> ctx,
                               std::ostream &stream, bool prev_category_exist,
                               std::vector<std::shared_ptr<Array>> arrays,
                               bool emit_type, bool ispc_type, bool emit_dims) {
    bool first = true;
    Options &options = Options::getInstance();
    for (auto &array : arrays) {
        if (!options.getAllowDeadData() && array->getIsDead())
            continue;
        if (std::find(pass_as_param_buffer.begin(), pass_as_param_buffer.end(),
                      array->getName(ctx)) == pass_as_param_buffer.end())
            continue;

        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        stream << placeSep(prev_category_exist || !first);
        if (emit_type)
            stream << array_type->getBaseType()->getName(ctx) << " ";
        stream << array->getName(ctx) << " ";
        if (emit_dims)
            for (const auto &dimension : array_type->getDimensions())
                stream << "[" << dimension << "] ";

        first = false;
    }
}

void emitSYCLBuffers(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                     std::string offset,
                     std::vector<std::shared_ptr<ScalarVar>> vars) {
    Options &options = Options::getInstance();
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        stream << offset << "buffer<";
        stream << var->getType()->getName(ctx);
        stream << ", 1> " << var->getName(ctx) << "_buf { ";
        stream << "&app_" << var->getName(ctx) << ", range<1>(1) };\n";
    }
}

void emitSYCLAccessors(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                       std::string offset,
                       std::vector<std::shared_ptr<ScalarVar>> vars,
                       bool is_inp) {
    Options &options = Options::getInstance();
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        stream << offset << "auto " << var->getName(ctx) << " = ";
        stream << var->getName(ctx) << "_buf.get_access<access::mode::";
        stream << (is_inp ? "read" : "write") << ">(cgh);\n";
    }
}

void ProgramGenerator::emitTest(std::shared_ptr<EmitCtx> ctx,
                                std::ostream &stream) {
    Options &options = Options::getInstance();
    stream << "#include \"init.h\"\n";
    if (options.isC()) {
        MinCall::emitCDefinition(ctx, stream);
        MaxCall::emitCDefinition(ctx, stream);
    }
    if (options.isCXX())
        stream << "#include <algorithm>\n";
    else if (options.isSYCL()) {
        stream << "#include <CL/sycl.hpp>\n";
        stream << "#if defined(FPGA) || defined(FPGA_EMULATOR)\n";
        stream << "    #include <CL/sycl/intel/fpga_extensions.hpp>\n";
        stream << "#endif\n";
    }

    if (options.isISPC()) {
        ctx->setIspcTypes(true);
        stream << "export ";
    }
    stream << "void test(";

    bool emit_any = emitVarFuncParam(ctx, stream, ext_inp_sym_tbl->getVars(),
                                     true, options.isISPC());

    emitArrayFuncParam(ctx, stream, emit_any, ext_inp_sym_tbl->getArrays(),
                       true, options.isISPC(), true);

    stream << ") ";

    if (options.isSYCL()) {
        stream << "{\n";
        stream << "    using namespace cl::sycl;\n\n";
        stream << "    {\n";

        stream << "#if defined(CPU)\n";
        stream << "        cpu_selector selector;\n";
        stream << "#elif defined(GPU)\n";
        stream << "        gpu_selector selector;\n";
        stream << "#elif defined(FPGA_EMULATOR)\n";
        stream << "        intel::fpga_emulator_selector selector;\n";
        stream << "#elif defined(FPGA)\n";
        stream << "        intel::fpga_selector selector;\n";
        stream << "#else\n";
        stream << "        default_selector selector;\n";
        stream << "#endif\n";
        stream << "        queue myQueue(selector);\n";
        emitSYCLBuffers(ctx, stream, "        ", ext_inp_sym_tbl->getVars());
        emitSYCLBuffers(ctx, stream, "        ", ext_out_sym_tbl->getVars());

        stream << "        myQueue.submit([&](handler & cgh) {\n";
        emitSYCLAccessors(ctx, stream, "            ",
                          ext_inp_sym_tbl->getVars(), true);
        emitSYCLAccessors(ctx, stream, "            ",
                          ext_out_sym_tbl->getVars(), false);
        stream << "            cgh.single_task<class test_func>([=] ()\n";
    }

    if (options.isSYCL())
        ctx->setSYCLAccess(true);
    new_test->emit(ctx, stream, !options.isSYCL() ? "" : "            ");

    if (options.isSYCL()) {
        stream << "            );\n";
        stream << "        });\n";
        stream << "    }\n";
        stream << "}\n";
    }
    ctx->setSYCLAccess(false);
    ctx->setIspcTypes(false);
}

void ProgramGenerator::emitMain(std::shared_ptr<EmitCtx> ctx,
                                std::ostream &stream) {
    Options &options = Options::getInstance();
    if (options.isISPC())
        stream << "extern \"C\" { ";

    stream << "void test(";

    bool emit_any =
        emitVarFuncParam(ctx, stream, ext_inp_sym_tbl->getVars(), true, false);
    emitArrayFuncParam(ctx, stream, emit_any, ext_inp_sym_tbl->getArrays(),
                       true, false, true);

    stream << ");";
    if (options.isISPC())
        stream << " }\n";
    stream << "\n\n";
    stream << "int main() {\n";
    stream << "    init();\n";
    stream << "    test(";

    emit_any =
        emitVarFuncParam(ctx, stream, ext_inp_sym_tbl->getVars(), false, false);

    emitArrayFuncParam(ctx, stream, emit_any, ext_inp_sym_tbl->getArrays(),
                       false, false, false);

    stream << ");\n";
    stream << "    checksum();\n";
    stream << "    printf(\"%llu\\n\", seed);\n";
    if (options.getCheckAlgo() == CheckAlgo::PRECOMPUTE) {
        stream << "    if (seed != " << hash_seed << "ULL) \n";
        stream << "        printf(\"ERROR: hash mismatch\\n\");\n";
    }
    if (options.getCheckAlgo() == CheckAlgo::ASSERTS) {
        stream << "    if (value_mismatch) \n";
        stream << "        printf(\"ERROR: value mismatch\\n\");\n";
    }
    stream << "}\n";
}

void ProgramGenerator::emit() {
    Options &options = Options::getInstance();
    auto emit_ctx = std::make_shared<EmitCtx>();
    // We need to narrow options if we were asked to do so
    if (options.getUniqueAlignSize() &&
        options.getAlignSize() == AlignmentSize::MAX_ALIGNMENT_SIZE) {
        AlignmentSize align_size = rand_val_gen->getRandId(
            emit_ctx->getEmitPolicy()->align_size_distr);
        options.setAlignSize(align_size);
    }

    std::ofstream out_file;

    // TODO: probably won't work on Windows
    std::string out_dir = options.getOutDir() + "/";

    auto open_file = [&out_file, &out_dir](std::string file_name) {
        out_file.open(out_dir + file_name);
        if (!out_file)
            ERROR(std::string("Can't open file ") + file_name);
    };

    open_file("init.h");
    emitExtDecl(emit_ctx, out_file);
    out_file.close();

    std::string func_file_ext, driver_file_ext;
    if (options.isC()) {
        func_file_ext = "c";
        driver_file_ext = "c";
    }
    else if (options.isCXX() || options.isSYCL()) {
        func_file_ext = "cpp";
        driver_file_ext = "cpp";
    }
    else if (options.isISPC()) {
        func_file_ext = "ispc";
        driver_file_ext = "cpp";
    }
    open_file("func." + func_file_ext);
    out_file << "/*\n";
    options.dump(out_file);
    out_file << "*/\n";
    emitTest(emit_ctx, out_file);
    out_file.close();

    open_file("driver." + driver_file_ext);
    emitCheckFunc(out_file);
    emitDecl(emit_ctx, out_file);
    emitInit(emit_ctx, out_file);
    emitCheck(emit_ctx, out_file);
    emitMain(emit_ctx, out_file);
    out_file.close();
}

void ProgramGenerator::hash(unsigned long long int const v) {
    // This function has to be exactly the same as the one that we use for hash
    // computation
    hash_seed ^= v + 0x9e3779b9 + (hash_seed << 6) + (hash_seed >> 2);
}

void ProgramGenerator::hashArray(std::shared_ptr<Array> const &arr) {
    assert(arr->getType()->isArrayType() && "Array should have array type");
    auto arr_type = std::static_pointer_cast<ArrayType>(arr->getType());
    std::vector<size_t> idx_vec(arr_type->getDimensions().size(), 0);
    auto &dims = arr_type->getDimensions();
    // TODO: this is broken now
    uint64_t init_val =
        arr->getInitValues(Options::main_val_idx).getAbsValue().value;
    uint64_t cur_val =
        arr->getCurrentValues(Options::main_val_idx).getAbsValue().value;
    std::vector<size_t> steps = {};
    hashArrayStep(arr, dims, idx_vec, 0, false, init_val, cur_val, steps);
}

void ProgramGenerator::hashArrayStep(std::shared_ptr<Array> const &arr,
                                     std::vector<size_t> &dims,
                                     std::vector<size_t> &idx_vec,
                                     size_t cur_idx, bool has_to_use_init_val,
                                     uint64_t &init_val, uint64_t &cur_val,
                                     std::vector<size_t> &steps) {
    size_t vec_last_idx = idx_vec.size() - 1;
    // TODO: this is also broken
    size_t cur_val_size = 0;
    size_t cur_dim = dims[cur_idx];
    size_t cur_step = steps[cur_idx];

    for (size_t i = 0; i < cur_dim; ++i) {
        has_to_use_init_val |= (i >= cur_val_size);
        if (cur_idx != vec_last_idx) {
            idx_vec[cur_idx] = i;
            hashArrayStep(arr, dims, idx_vec, cur_idx + 1,
                          has_to_use_init_val || (i % cur_step != 0), init_val,
                          cur_val, steps);
        }
        else {
            hash(has_to_use_init_val || (i % cur_step != 0) ? init_val
                                                            : cur_val);
        }
    }
}
