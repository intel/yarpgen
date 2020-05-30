/*
Copyright (c) 2015-2017, Intel Corporation
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

static void emitVarsDecl(std::ostream &stream,
                         std::vector<std::shared_ptr<ScalarVar>> vars) {
    Options &options = Options::getInstance();
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        auto init_val = std::make_shared<ConstantExpr>(var->getInitValue());
        auto decl_stmt = std::make_shared<DeclStmt>(var, init_val);
        decl_stmt->emit(stream);
        stream << "\n";
    }
}

static void emitArrayDecl(std::ostream &stream,
                          std::vector<std::shared_ptr<Array>> arrays) {
    Options &options = Options::getInstance();
    for (auto &array : arrays) {
        if (!options.getAllowDeadData() && array->getIsDead())
            continue;
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        stream << array_type->getBaseType()->getName() << " ";
        stream << array->getName() << " ";
        for (const auto &dimension : array_type->getDimensions()) {
            stream << "[" << dimension << "] ";
        }
        stream << ";\n";
    }
}

void ProgramGenerator::emitDecl(std::ostream &stream) {
    emitVarsDecl(stream, ext_inp_sym_tbl->getVars());
    emitVarsDecl(stream, ext_out_sym_tbl->getVars());

    emitArrayDecl(stream, ext_inp_sym_tbl->getArrays());
    emitArrayDecl(stream, ext_out_sym_tbl->getArrays());
}

static void emitArrayInit(std::ostream &stream,
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
        stream << offset << array->getName() << " ";
        for (size_t i = 0; i < idx; ++i)
            stream << "[i_" << i << "] ";
        stream << "= ";
        auto init_var = array->getInitValues();
        assert(init_var->getKind() == DataKind::VAR &&
               "We support simple array for now");
        auto init_scalar_var = std::static_pointer_cast<ScalarVar>(init_var);
        auto init_const =
            std::make_shared<ConstantExpr>(init_scalar_var->getInitValue());
        init_const->emit(stream);
        stream << ";\n";
    }
}

void ProgramGenerator::emitInit(std::ostream &stream) {
    stream << "void init() {\n";
    emitArrayInit(stream, ext_inp_sym_tbl->getArrays());
    emitArrayInit(stream, ext_out_sym_tbl->getArrays());
    stream << "}\n\n";
}

void ProgramGenerator::emitCheck(std::ostream &stream) {
    stream << "void checksum() {\n";

    Options &options = Options::getInstance();

    EmitPolicy emit_pol;
    for (auto &var : ext_out_sym_tbl->getVars()) {
        bool use_assert = false;
        if (options.useAsserts() == OptionLevel::SOME)
            use_assert = rand_val_gen->getRandId(emit_pol.asserts_check_distr);
        else if (options.useAsserts() == OptionLevel::ALL)
            use_assert = true;

        if (!use_assert)
            stream << "    hash(&seed, " << var->getName() << ");\n";
        else {
            auto const_val =
                std::make_shared<ConstantExpr>(var->getCurrentValue());
            stream << "    assert(" << var->getName() << " == ";
            const_val->emit(stream);
            stream << ");\n";
        }
    }

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
            use_assert = rand_val_gen->getRandId(emit_pol.asserts_check_distr);
        else if (options.useAsserts() == OptionLevel::ALL)
            use_assert = true;

        if (!use_assert)
            stream << offset << "hash(&seed, ";
        else
            stream << "    assert(";
        std::stringstream ss;
        ss << array->getName() << " ";
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
            const_val->emit(stream);
            stream << " || " << arr_name << " == ";
            if (!array->getInitValues()->isScalarVar())
                ERROR("We support only scalar variables for now");
            const_val = std::make_shared<ConstantExpr>(
                std::static_pointer_cast<ScalarVar>(array->getInitValues())
                    ->getCurrentValue());
            const_val->emit(stream);
        }
        stream << ");\n";
    }
    stream << "}\n";
}

// This buffer tracks what input data we pass as a parameters to test functions
static std::vector<std::string> pass_as_param_buffer;
static bool any_vars_as_params = false;
static bool any_arrays_as_params = false;

static void emitVarExtDecl(std::ostream &stream,
                           std::vector<std::shared_ptr<ScalarVar>> vars,
                           bool inp_category) {
    EmitPolicy emit_pol;
    Options &options = Options::getInstance();
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        bool pass_as_param = false;
        if (inp_category) {
            if (options.inpAsArgs() == OptionLevel::SOME)
                pass_as_param =
                    rand_val_gen->getRandId(emit_pol.pass_as_param_distr);
            else if (options.inpAsArgs() == OptionLevel::ALL)
                pass_as_param = true;
        }

        if (pass_as_param) {
            pass_as_param_buffer.push_back(var->getName());
            any_vars_as_params = true;
            continue;
        }
        stream << "extern ";
        stream << (options.isISPC() ? var->getType()->getIspcName()
                                    : var->getType()->getName());
        stream << " " << var->getName() << ";\n";
    }
}

static void emitArrayExtDecl(std::ostream &stream,
                             std::vector<std::shared_ptr<Array>> arrays,
                             bool inp_category) {
    EmitPolicy emit_pol;
    Options &options = Options::getInstance();
    for (auto &array : arrays) {
        if (!options.getAllowDeadData() && array->getIsDead())
            continue;
        bool pass_as_param = false;
        if (inp_category) {
            if (options.inpAsArgs() == OptionLevel::SOME)
                pass_as_param =
                    rand_val_gen->getRandId(emit_pol.pass_as_param_distr);
            else if (options.inpAsArgs() == OptionLevel::ALL)
                pass_as_param = true;
        }

        if (pass_as_param) {
            pass_as_param_buffer.push_back(array->getName());
            any_arrays_as_params = true;
            continue;
        }

        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        stream << "extern ";
        stream << (options.isISPC() ? array_type->getBaseType()->getIspcName()
                                    : array_type->getBaseType()->getName());
        stream << " ";
        stream << array->getName() << " ";
        for (const auto &dimension : array_type->getDimensions()) {
            stream << "[" << dimension << "] ";
        }

        if (options.getEmitAlignAttr() != OptionLevel::NONE) {
            bool emit_align_attr = true;
            if (options.getEmitAlignAttr() == OptionLevel::SOME)
                emit_align_attr =
                    rand_val_gen->getRandId(emit_pol.emit_align_attr_distr);
            if (emit_align_attr) {
                AlignmentSize align_size = options.getAlignSize();
                if (!options.getUniqueAlignSize())
                    align_size =
                        rand_val_gen->getRandId(emit_pol.align_size_distr);
                stream << "__attribute__((aligned(";
                switch (align_size) {
                    case AlignmentSize::A16:
                        stream << "16";
                        break;
                    case AlignmentSize::A32:
                        stream << "32";
                        break;
                    case AlignmentSize::A64:
                        stream << "64";
                        break;
                    case AlignmentSize::MAX_ALIGNMENT_SIZE:
                        ERROR("Bad alignment size");
                }
                stream << ")))";
            }
        }

        stream << ";\n";
    }
}

void ProgramGenerator::emitExtDecl(std::ostream &stream) {
    emitVarExtDecl(stream, ext_inp_sym_tbl->getVars(), true);
    emitVarExtDecl(stream, ext_out_sym_tbl->getVars(), false);
    emitArrayExtDecl(stream, ext_inp_sym_tbl->getArrays(), true);
    emitArrayExtDecl(stream, ext_out_sym_tbl->getArrays(), false);
}

static std::string placeSep(bool cond) { return cond ? ", " : ""; }

static bool emitVarFuncParam(std::ostream &stream,
                             std::vector<std::shared_ptr<ScalarVar>> vars,
                             bool emit_type, bool ispc_type) {
    bool emit_any = false;
    Options &options = Options::getInstance();
    for (auto &var : vars) {
        if (!options.getAllowDeadData() && var->getIsDead())
            continue;
        if (std::find(pass_as_param_buffer.begin(), pass_as_param_buffer.end(),
                      var->getName()) == pass_as_param_buffer.end())
            continue;

        stream << placeSep(emit_any);
        if (emit_type)
            stream << (ispc_type ? var->getType()->getIspcName()
                                 : var->getType()->getName())
                   << " ";
        stream << var->getName();
        emit_any = true;
    }
    return emit_any;
}

static void emitArrayFuncParam(std::ostream &stream, bool prev_category_exist,
                               std::vector<std::shared_ptr<Array>> arrays,
                               bool emit_type, bool ispc_type, bool emit_dims) {
    bool first = true;
    Options &options = Options::getInstance();
    for (auto &array : arrays) {
        if (!options.getAllowDeadData() && array->getIsDead())
            continue;
        if (std::find(pass_as_param_buffer.begin(), pass_as_param_buffer.end(),
                      array->getName()) == pass_as_param_buffer.end())
            continue;

        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        stream << placeSep(prev_category_exist || !first);
        if (emit_type)
            stream << (ispc_type ? array_type->getBaseType()->getIspcName()
                                 : array_type->getBaseType()->getName())
                   << " ";
        stream << array->getName() << " ";
        if (emit_dims)
            for (const auto &dimension : array_type->getDimensions())
                stream << "[" << dimension << "] ";

        first = false;
    }
}

void ProgramGenerator::emitTest(std::ostream &stream) {
    Options &options = Options::getInstance();
    stream << "#include \"init.h\"\n";
    if (!options.isISPC()) {
        stream << "#include <algorithm>\n";
    }
    if (options.isISPC())
        stream << "export ";
    stream << "void test(";

    bool emit_any = emitVarFuncParam(stream, ext_inp_sym_tbl->getVars(), true,
                                     options.isISPC());

    emitArrayFuncParam(stream, emit_any, ext_inp_sym_tbl->getArrays(), true,
                       options.isISPC(), true);

    stream << ") ";
    new_test->emit(stream);
}

void ProgramGenerator::emitMain(std::ostream &stream) {
    Options &options = Options::getInstance();
    if (options.isISPC())
        stream << "extern \"C\" { ";

    stream << "void test(";

    bool emit_any =
        emitVarFuncParam(stream, ext_inp_sym_tbl->getVars(), true, false);
    emitArrayFuncParam(stream, emit_any, ext_inp_sym_tbl->getArrays(), true,
                       false, true);

    stream << ");";
    if (options.isISPC())
        stream << " }\n";
    stream << "int main() {\n";
    stream << "    init();\n";
    stream << "    test(";

    emit_any =
        emitVarFuncParam(stream, ext_inp_sym_tbl->getVars(), false, false);

    emitArrayFuncParam(stream, emit_any, ext_inp_sym_tbl->getArrays(), false,
                       false, false);

    stream << ");\n";
    stream << "    checksum();\n";
    stream << "    printf(\"%llu\\n\", seed);\n";
    stream << "}\n";
}

void ProgramGenerator::emit() {
    Options &options = Options::getInstance();

    // We need to narrow options if we were asked to do so
    if (options.getUniqueAlignSize() &&
        options.getAlignSize() == AlignmentSize::MAX_ALIGNMENT_SIZE) {
        EmitPolicy emit_pol;
        AlignmentSize align_size =
            rand_val_gen->getRandId(emit_pol.align_size_distr);
        options.setAlignSize(align_size);
    }

    std::ofstream out_file;

    out_file.open("init.h");
    emitExtDecl(out_file);
    out_file.close();

    out_file.open(!options.isISPC() ? "func.cpp" : "func.ispc");
    emitTest(out_file);
    out_file.close();

    out_file.open("driver.cpp");
    emitCheckFunc(out_file);
    emitDecl(out_file);
    emitInit(out_file);
    emitCheck(out_file);
    emitMain(out_file);
    out_file.close();
}
