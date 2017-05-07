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

#include "master.h"

///////////////////////////////////////////////////////////////////////////////

using namespace yarpgen;

Master::Master (std::string _out_folder) {
    out_folder = _out_folder;
    extern_inp_sym_table = std::make_shared<SymbolTable> ();
    extern_mix_sym_table = std::make_shared<SymbolTable> ();
    extern_out_sym_table = std::make_shared<SymbolTable> ();
}

void Master::generate () {
    Context ctx (gen_policy, nullptr, Node::NodeID::MAX_STMT_ID, true);
    ctx.set_extern_inp_sym_table (extern_inp_sym_table);
    ctx.set_extern_mix_sym_table (extern_mix_sym_table);
    ctx.set_extern_out_sym_table (extern_out_sym_table);

    program = ScopeStmt::generate(std::make_shared<Context>(ctx));
}

void Master::write_file (std::string of_name, std::string data) {
    std::ofstream out_file;
    out_file.open (out_folder + "/" + of_name);
    out_file << data;
    out_file.close ();
}

std::string Master::emit_init () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";

    ret += extern_inp_sym_table->emit_variable_def() + "\n\n";
    ret += extern_mix_sym_table->emit_variable_def() + "\n\n";
    ret += extern_out_sym_table->emit_variable_def() + "\n\n";
    ret += extern_inp_sym_table->emit_struct_def() + "\n\n";
    ret += extern_mix_sym_table->emit_struct_def() + "\n\n";
    ret += extern_out_sym_table->emit_struct_def() + "\n\n";
    //TODO: what if we extand struct types in mix_sym_table and out_sym_table
    ret += extern_inp_sym_table->emit_struct_type_static_memb_def() + "\n\n";

    ret += "void init () {\n";
    ret += extern_inp_sym_table->emit_struct_init ("    ");
    ret += extern_mix_sym_table->emit_struct_init ("    ");
    ret += extern_out_sym_table->emit_struct_init ("    ");
    ret += "}";

    write_file("init.cpp", ret);
    return ret;
}

std::string Master::emit_decl () {
    std::string ret = "";
    /* TODO: none of it is used currently.
     * All these headers must be added only when they are really needed.
     * Parsing these headers is costly for compile time
    ret += "#include <cstdint>\n";
    ret += "#include <array>\n";
    ret += "#include <vector>\n";
    ret += "#include <valarray>\n\n";
    */

    ret += "void hash(unsigned long long int &seed, unsigned long long int const &v);\n\n";

    ret += extern_inp_sym_table->emit_variable_extern_decl() + "\n\n";
    ret += extern_mix_sym_table->emit_variable_extern_decl() + "\n\n";
    ret += extern_out_sym_table->emit_variable_extern_decl() + "\n\n";
    //TODO: what if we extand struct types in mix_sym_tabl
    ret += extern_inp_sym_table->emit_struct_type_def() + "\n\n";
    ret += extern_inp_sym_table->emit_struct_extern_decl() + "\n\n";
    ret += extern_mix_sym_table->emit_struct_extern_decl() + "\n\n";
    ret += extern_out_sym_table->emit_struct_extern_decl() + "\n\n";

    write_file("init.h", ret);
    return ret;
}

std::string Master::emit_func () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";
    ret += "void foo () {\n";
    ret += program->emit();
    ret += "}";
    write_file("func.cpp", ret);
    return ret;
}

std::string Master::emit_hash () {
    std::string ret = "#include <functional>\n";
    ret += "void hash(unsigned long long int &seed, unsigned long long int const &v) {\n";
    ret += "    seed ^= v + 0x9e3779b9 + (seed<<6) + (seed>>2);\n";
    ret += "}\n";
    write_file("hash.cpp", ret);
    return ret;
}

std::string Master::emit_check () { // TODO: rewrite with IR
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";

    ret += "unsigned long long int checksum () {\n";

    std::shared_ptr<ScalarVariable> seed = std::make_shared<ScalarVariable>("seed", IntegerType::init(Type::IntegerTypeID::ULLINT));
    std::shared_ptr<VarUseExpr> seed_use = std::make_shared<VarUseExpr>(seed);

    AtomicType::ScalarTypedVal zero_init (IntegerType::IntegerTypeID::ULLINT);
    zero_init.val.ullint_val = 0;
    std::shared_ptr<ConstExpr> const_init = std::make_shared<ConstExpr> (zero_init);

    std::shared_ptr<DeclStmt> seed_decl = std::make_shared<DeclStmt>(seed, const_init);

    ret += seed_decl->emit("    ") + "\n";

    ret += extern_mix_sym_table->emit_variable_check ("    ");
    ret += extern_out_sym_table->emit_variable_check ("    ");

    ret += extern_mix_sym_table->emit_struct_check ("    ");
    ret += extern_out_sym_table->emit_struct_check ("    ");

    ret += "    return seed;\n";
    ret += "}";
    write_file("check.cpp", ret);
    return ret;
}

std::string Master::emit_main () {
    std::string ret = "";
    ret += "#include <iostream>\n";
    ret += "#include \"init.h\"\n\n";
    ret += "extern void init ();\n";
    ret += "extern void foo ();\n";
    ret += "extern unsigned long long int checksum ();\n\n";
    ret += "int main () {\n";
    ret += "    init ();\n";
    ret += "    foo ();\n";
    ret += "    std::cout << checksum () << std::endl;\n";
    ret += "    return 0;\n";
    ret += "}";
    write_file("driver.cpp", ret);
    return ret;
}

