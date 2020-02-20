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
#include "stmt.h"
#include "data.h"
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

    // Create some number of ScalarVariables that we will use to provide input data to the test program
    size_t inp_vars_num = rand_val_gen->getRandValue(gen_pol->min_inp_vars_num, gen_pol->max_inp_vars_num);
    for (size_t i = 0; i < inp_vars_num; ++i) {
        auto new_var = ScalarVar::create(pop_ctx);
        ext_inp_sym_tbl->addVar(new_var);
        ext_inp_sym_tbl->addVarExpr(std::make_shared<ScalarVarUseExpr>(new_var));
    }

    pop_ctx->setExtInpSymTable(ext_inp_sym_tbl);
    pop_ctx->setExtOutSymTable(ext_out_sym_tbl);

    new_test->populate(pop_ctx);
}

void ProgramGenerator::emitCheckFunc(std::ostream &stream) {
    std::ostream& out_file = stream;
    out_file << "#include <stdio.h>\n\n";
    out_file << "unsigned long long int seed = 0;\n";
    out_file << "void hash(unsigned long long int *seed, unsigned long long int const v) {\n";
    out_file << "    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);\n";
    out_file << "}\n\n";
}

void ProgramGenerator::emitDecl(std::ostream &stream) {
    //TODO: create functions to reduce the amount of copy-pasted code
    for (auto &var : ext_inp_sym_tbl->getVars()) {
        auto init_val = std::make_shared<ConstantExpr>(var->getInitValue());
        auto decl_stmt = std::make_shared<DeclStmt>(var, init_val);
        decl_stmt->emit(stream);
        stream << "\n";
    }
    for (auto &var : ext_out_sym_tbl->getVars()) {
        auto init_val = std::make_shared<ConstantExpr>(var->getInitValue());
        auto decl_stmt = std::make_shared<DeclStmt>(var, init_val);
        decl_stmt->emit(stream);
        stream << "\n";
    }
    for (auto &array : ext_inp_sym_tbl->getArrays()) {
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
    for (auto &array : ext_out_sym_tbl->getArrays()) {
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

void ProgramGenerator::emitInit(std::ostream &stream) {
    stream << "void init() {\n";
    for (const auto &array : ext_inp_sym_tbl->getArrays()) {
        std::string offset = "    ";
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        size_t idx = 0;
        for (const auto &dimension : array_type->getDimensions()) {
            stream << offset << "for (size_t i_" << idx << " = 0; i_" << idx << " < " << dimension << "; ++i_" << idx << ") \n";
            offset += "    ";
            idx++;
        }
        stream << offset << array->getName() << " ";
        for (size_t i = 0; i < idx; ++i)
            stream << "[i_" << i << "] ";
        stream << "= ";
        auto init_var = array->getInitValues();
        assert(init_var->getKind() == DataKind::VAR && "We support simple array for now");
        auto init_scalar_var = std::static_pointer_cast<ScalarVar>(init_var);
        auto init_const = std::make_shared<ConstantExpr>(init_scalar_var->getInitValue());
        init_const->emit(stream);
        stream << ";\n";
    }

    for (const auto &array : ext_out_sym_tbl->getArrays()) {
        std::string offset = "    ";
        auto type = array->getType();
        assert(type->isArrayType() && "Array should have an Array type");
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        size_t idx = 0;
        for (const auto &dimension : array_type->getDimensions()) {
            stream << offset << "for (size_t i_" << idx << " = 0; i_" << idx << " < " << dimension << "; ++i_" << idx << ") \n";
            offset += "    ";
            idx++;
        }
        stream << offset << array->getName() << " ";
        for (size_t i = 0; i < idx; ++i)
            stream << "[i_" << i << "] ";
        stream << "= ";
        auto init_var = array->getInitValues();
        assert(init_var->getKind() == DataKind::VAR && "We support simple array for now");
        auto init_scalar_var = std::static_pointer_cast<ScalarVar>(init_var);
        auto init_const = std::make_shared<ConstantExpr>(init_scalar_var->getInitValue());
        init_const->emit(stream);
        stream << ";\n";
    }

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
            stream << offset << "for (size_t i_" << idx << " = 0; i_" << idx << " < " << dimension << "; ++i_" << idx << ") \n";
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

void ProgramGenerator::emitTest(std::ostream &stream) {
    stream << "void test() {\n";
    new_test->emit(stream, "    ");
    stream << "}\n";
}

void ProgramGenerator::emitMain(std::ostream &stream) {
    stream << "int main() {\n";
    stream << "    init();\n";
    stream << "    test();\n";
    stream << "    checksum();\n";
    stream << "    printf(\"%llu\\n\", seed);\n";
    stream << "}\n";
}

void ProgramGenerator::emit(std::ostream &stream) {
    emitCheckFunc(stream);
    emitDecl(stream);
    emitInit(stream);
    emitCheck(stream);
    emitTest(stream);
    emitMain(stream);
}
