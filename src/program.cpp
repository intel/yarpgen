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
#include "stmt.h"
#include <fstream>
#include <memory>

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
    out_file << "unsigned long long int seed = 0;\n";
    out_file << "void hash(unsigned long long int *seed, unsigned long long "
                "int const v) {\n";
    out_file << "    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);\n";
    out_file << "}\n\n";
}

static void emitVarsDecl(std::ostream &stream,
                         std::vector<std::shared_ptr<ScalarVar>> vars) {
    for (auto &var : vars) {
        auto init_val = std::make_shared<ConstantExpr>(var->getInitValue());
        auto decl_stmt = std::make_shared<DeclStmt>(var, init_val);
        decl_stmt->emit(stream);
        stream << "\n";
    }
}

static void emitArrayDecl(std::ostream &stream,
                          std::vector<std::shared_ptr<Array>> arrays) {
    for (auto &array : arrays) {
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
    for (const auto &array : arrays) {
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

    for (auto &var : ext_out_sym_tbl->getVars()) {
        stream << "    hash(&seed, " << var->getName() << ");\n";
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
        stream << offset << "hash(&seed, " << array->getName() << " ";
        for (size_t i = 0; i < idx; ++i)
            stream << "[i_" << i << "] ";
        stream << ");\n";
    }
    stream << "}\n";
}

static void emitVarExtDecl(std::ostream &stream,
                           std::vector<std::shared_ptr<ScalarVar>> vars) {
    for (auto &var : vars) {
        stream << "extern " << var->getType()->getName() << " "
               << var->getName() << ";\n";
    }
}

static void emitArrayExtDecl(std::ostream &stream,
                             std::vector<std::shared_ptr<Array>> arrays) {
    for (auto &array : arrays) {
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        stream << "extern " << array_type->getBaseType()->getName() << " ";
        stream << array->getName() << " ";
        for (const auto &dimension : array_type->getDimensions()) {
            stream << "[" << dimension << "] ";
        }
        stream << ";\n";
    }
}

void ProgramGenerator::emitExtDecl(std::ostream &stream) {
    emitVarExtDecl(stream, ext_inp_sym_tbl->getVars());
    emitVarExtDecl(stream, ext_out_sym_tbl->getVars());
    emitArrayExtDecl(stream, ext_inp_sym_tbl->getArrays());
    emitArrayExtDecl(stream, ext_out_sym_tbl->getArrays());
}

static std::string placeSep(bool cond) { return cond ? ", " : ""; }

static void emitIspcVarFuncParam(std::ostream &stream,
                                 std::vector<std::shared_ptr<ScalarVar>> vars,
                                 bool emit_type, bool ispc_type) {
    for (auto &var : vars) {
        if (emit_type)
            stream << (ispc_type ? var->getType()->getIspcName()
                                 : var->getType()->getName())
                   << " ";
        stream << var->getName();
        stream << placeSep(var != *(vars.end() - 1));
    }
}

static void emitIspcArrayFuncParam(std::ostream &stream,
                                   std::vector<std::shared_ptr<Array>> arrays,
                                   bool emit_type, bool ispc_type,
                                   bool emit_dims) {
    for (auto &array : arrays) {
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        if (emit_type)
            stream << (ispc_type ? array_type->getBaseType()->getIspcName()
                                 : array_type->getBaseType()->getName())
                   << " ";
        stream << array->getName() << " ";
        if (emit_dims)
            for (const auto &dimension : array_type->getDimensions())
                stream << "[" << dimension << "] ";
        stream << placeSep(array != *(arrays.end() - 1));
    }
}

void ProgramGenerator::emitTest(std::ostream &stream) {
    Options &options = Options::getInstance();
    if (!options.isISPC()) {
        stream << "#include \"init.h\"\n";
        stream << "#include <algorithm>\n";
        stream << "void test() ";
    }
    else {
        stream << "export void test(";

        auto place_cat_sep = [&stream](auto next_group) {
            if (!next_group.empty())
                stream << ", ";
        };

        emitIspcVarFuncParam(stream, ext_inp_sym_tbl->getVars(), true, true);
        place_cat_sep(ext_out_sym_tbl->getVars());
        emitIspcVarFuncParam(stream, ext_out_sym_tbl->getVars(), true, true);
        place_cat_sep(ext_inp_sym_tbl->getArrays());

        emitIspcArrayFuncParam(stream, ext_inp_sym_tbl->getArrays(), true, true,
                               true);
        place_cat_sep(ext_out_sym_tbl->getArrays());
        emitIspcArrayFuncParam(stream, ext_out_sym_tbl->getArrays(), true, true,
                               true);

        stream << ") ";
    }
    new_test->emit(stream);
}

void ProgramGenerator::emitMain(std::ostream &stream) {
    Options &options = Options::getInstance();
    if (!options.isISPC())
        stream << "void test();\n";
    else {
        stream << "extern \"C\" { void test(";

        auto place_cat_sep = [&stream](auto next_group) {
            if (!next_group.empty())
                stream << ", ";
        };

        emitIspcVarFuncParam(stream, ext_inp_sym_tbl->getVars(), true, false);
        place_cat_sep(ext_out_sym_tbl->getVars());
        emitIspcVarFuncParam(stream, ext_out_sym_tbl->getVars(), true, false);
        place_cat_sep(ext_inp_sym_tbl->getArrays());

        emitIspcArrayFuncParam(stream, ext_inp_sym_tbl->getArrays(), true,
                               false, true);
        place_cat_sep(ext_out_sym_tbl->getArrays());
        emitIspcArrayFuncParam(stream, ext_out_sym_tbl->getArrays(), true,
                               false, true);

        stream << "); }\n";
    }
    stream << "int main() {\n";
    stream << "    init();\n";
    if (!options.isISPC())
        stream << "    test();\n";
    else {
        stream << "    test(";
        auto place_cat_sep = [&stream](auto next_group) {
            if (!next_group.empty())
                stream << ", ";
        };

        emitIspcVarFuncParam(stream, ext_inp_sym_tbl->getVars(), false, false);
        place_cat_sep(ext_out_sym_tbl->getVars());
        emitIspcVarFuncParam(stream, ext_out_sym_tbl->getVars(), false, false);
        place_cat_sep(ext_inp_sym_tbl->getArrays());

        emitIspcArrayFuncParam(stream, ext_inp_sym_tbl->getArrays(), false,
                               false, false);
        place_cat_sep(ext_out_sym_tbl->getArrays());
        emitIspcArrayFuncParam(stream, ext_out_sym_tbl->getArrays(), false,
                               false, false);

        stream << ");\n";
    }
    stream << "    checksum();\n";
    stream << "    printf(\"%llu\\n\", seed);\n";
    stream << "}\n";
}

void ProgramGenerator::emit() {
    std::ofstream out_file;
    out_file.open("driver.cpp");
    emitCheckFunc(out_file);
    emitDecl(out_file);
    emitInit(out_file);
    emitCheck(out_file);
    emitMain(out_file);
    out_file.close();

    Options &options = Options::getInstance();
    out_file.open(!options.isISPC() ? "func.cpp" : "func.ispc");
    emitTest(out_file);
    out_file.close();

    out_file.open("init.h");
    emitExtDecl(out_file);
    out_file.close();
}
