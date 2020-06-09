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

ProgramGenerator::ProgramGenerator() {
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
}

void ProgramGenerator::emitCheckFunc(std::ostream &stream) {
    std::ostream &out_file = stream;
    out_file << "#include <stdio.h>\n\n";

    Options &options = Options::getInstance();
    if (options.useAsserts() != OptionLevel::NONE)
        stream << "#include <cassert>\n\n";

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
        auto init_var = array->getInitValues();
        assert(init_var->getKind() == DataKind::VAR &&
               "We support simple array for now");
        auto init_scalar_var = std::static_pointer_cast<ScalarVar>(init_var);
        auto init_const =
            std::make_shared<ConstantExpr>(init_scalar_var->getInitValue());
        init_const->emit(ctx, stream);
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
        bool use_assert = false;
        if (options.useAsserts() == OptionLevel::SOME)
            use_assert = rand_val_gen->getRandId(emit_pol->asserts_check_distr);
        else if (options.useAsserts() == OptionLevel::ALL)
            use_assert = true;

        std::string var_name = var->getName(ctx);

        if (!use_assert) {
            stream << "    hash(&seed, " << var_name << ");\n";
        }
        else {
            auto const_val =
                std::make_shared<ConstantExpr>(var->getCurrentValue());
            stream << "    assert(" << var_name << " == ";
            const_val->emit(ctx, stream);
            stream << ");\n";
        }
    }

    ctx->setSYCLPrefix("");

    for (const auto &array : ext_out_sym_tbl->getArrays()) {
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

        bool use_assert = false;
        if (options.useAsserts() == OptionLevel::SOME)
            use_assert = rand_val_gen->getRandId(emit_pol->asserts_check_distr);
        else if (options.useAsserts() == OptionLevel::ALL)
            use_assert = true;

        if (!use_assert)
            stream << offset << "hash(&seed, ";
        else
            stream << "    assert(";
        std::stringstream ss;
        ss << array->getName(ctx) << " ";
        for (size_t i = 0; i < idx; ++i)
            ss << "[i_" << i << "] ";
        std::string arr_name = ss.str();
        stream << arr_name;

        if (use_assert) {
            if (!array->getCurrentValues()->isScalarVar())
                ERROR("We support only scalar variables for now");
            auto const_val = std::make_shared<ConstantExpr>(
                std::static_pointer_cast<ScalarVar>(array->getCurrentValues())
                    ->getCurrentValue());
            stream << "== ";
            const_val->emit(ctx, stream);
            stream << " || " << arr_name << " == ";
            if (!array->getInitValues()->isScalarVar())
                ERROR("We support only scalar variables for now");
            const_val = std::make_shared<ConstantExpr>(
                std::static_pointer_cast<ScalarVar>(array->getInitValues())
                    ->getCurrentValue());
            const_val->emit(ctx, stream);
        }
        stream << ");\n";
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
    if (options.isCXX())
        stream << "#include <algorithm>\n";
    else if (options.isSYCL())
        stream << "#include <CL/sycl.hpp>\n";

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
        stream << "        queue myQueue;\n";
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

    out_file.open("init.h");
    emitExtDecl(emit_ctx, out_file);
    out_file.close();

    out_file.open(!options.isISPC() ? "func.cpp" : "func.ispc");
    emitTest(emit_ctx, out_file);
    out_file.close();

    out_file.open("driver.cpp");
    emitCheckFunc(out_file);
    emitDecl(emit_ctx, out_file);
    emitInit(emit_ctx, out_file);
    emitCheck(emit_ctx, out_file);
    emitMain(emit_ctx, out_file);
    out_file.close();
}
