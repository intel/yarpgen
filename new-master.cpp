/*
Copyright (c) 2015-2016, Intel Corporation

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

#include "new-master.h"

///////////////////////////////////////////////////////////////////////////////

Master::Master (std::string _out_folder) {
    out_folder = _out_folder;
    SymbolTable extrern_inp;
    extern_inp_sym_table = std::make_shared<SymbolTable> (extrern_inp);
    SymbolTable extrern_out;
    extern_out_sym_table = std::make_shared<SymbolTable> (extrern_out);
}

void Master::generate () {
    int inp_var_num = rand_val_gen->get_rand_value<int>(5, 10);
/*
    for (int i = 0; i < inp_var_num; ++i) {
        ScalarVariableGen scalar_var_gen (std::make_shared<GenPolicy>(gen_policy));
        scalar_var_gen.generate ();
        extern_inp_sym_table.add_variable (std::static_pointer_cast<Variable>(scalar_var_gen.get_data()));
    }
    int out_var_num = rand_val_gen->get_rand_value<int>(5, 10);
    for (int i = 0; i < out_var_num; ++i) {
        ScalarVariableGen scalar_var_gen (std::make_shared<GenPolicy>(gen_policy));
        scalar_var_gen.generate ();
        extern_out_sym_table.add_variable (std::static_pointer_cast<Variable>(scalar_var_gen.get_data()));
    }
*/
    Context ctx (gen_policy, NULL, Node::NodeID::MAX_STMT_ID, NULL, NULL);
//    std::cerr << "DEBUG OUTSIDE: init " << ((uint64_t) (&(*ctx.get_extern_inp_sym_table()))) << std::endl;
//    std::shared_ptr<SymbolTable> inp_st_ptr = std::make_shared<SymbolTable>(extern_inp_sym_table);
//    std::cerr << "DEBUG OUTSIDE: st_ptr " << ((uint64_t) (&(*inp_st_ptr))) << std::endl;
    ctx.set_extern_inp_sym_table (extern_inp_sym_table);
    ctx.set_extern_out_sym_table (extern_out_sym_table);

//    std::shared_ptr<Context> ctx_ptr = std::make_shared<Context> (ctx);
//    std::cerr << "DEBUG OUTSIDE: obj " << ((uint64_t) (&(*ctx.get_extern_inp_sym_table()))) << std::endl;
//    std::cerr << "DEBUG OUTSIDE: ptr " << ((uint64_t) (&(*ctx_ptr->get_extern_inp_sym_table()))) << std::endl;
    ScopeGen scope_gen (std::make_shared<Context> (ctx));
    scope_gen.generate();
//    std::cerr << "DEBUG OUTSIDE: ctx size " << ctx.get_extern_inp_sym_table()->get_variables().size() << std::endl;
//    std::cerr << "DEBUG OUTSIDE: st_ptr size " << inp_st_ptr->get_variables().size() << std::endl;
//    std::cerr << "DEBUG OUTSIDE: st " << ((uint64_t) (&(extern_inp_sym_table))) << std::endl;
//    std::cerr << "DEBUG OUTSIDE: st size " << extern_inp_sym_table.get_variables().size() << std::endl;
    program = scope_gen.get_scope ();
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

    ret += extern_inp_sym_table->emit_variable_def();
    ret += extern_out_sym_table->emit_variable_def();

    ret += "void init () {\n";
    ret += "}";

    write_file("init.cpp", ret);
    return ret;
}

std::string Master::emit_decl () {
    std::string ret = "";
    ret += "#include <cstdint>\n";
    ret += "#include <iostream>\n";
    ret += "#include <array>\n";
    ret += "#include <vector>\n";
    ret += "#include <valarray>\n";

    ret += "void hash(unsigned long long int &seed, unsigned long long int const &v);\n";

    ret += extern_inp_sym_table->emit_variable_extern_decl();
    ret += extern_out_sym_table->emit_variable_extern_decl();

    write_file("init.h", ret);
    return ret;
}

std::string Master::emit_func () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";
    ret += "void foo () {\n";
    for (auto i = program.begin(); i != program.end(); ++i) {
        ret += (*i)->emit() + "\n";
    }
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

    Variable seed = Variable("seed", IntegerType::IntegerTypeID::ULLINT, Variable::Mod::NTHNG, false);
    VarUseExpr seed_use;
    seed_use.set_variable (std::make_shared<Variable> (seed));

    ConstExpr zero_init;
    zero_init.set_type (IntegerType::IntegerTypeID::ULLINT);
    zero_init.set_data (0);

    DeclStmt seed_decl;
    seed_decl.set_data (std::make_shared<Variable> (seed));
    seed_decl.set_init (std::make_shared<ConstExpr> (zero_init));

    ret += seed_decl.emit() + ";\n";

    ret += extern_out_sym_table->emit_variable_check ();

    ret += "    return seed;\n";
    ret += "}";
    write_file("check.cpp", ret);
    return ret;
}

std::string Master::emit_main () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";
    ret += "extern void init ();\n";
    ret += "extern void foo ();\n";
    ret += "extern unsigned long long int checksum ();\n";
    ret += "int main () {\n";
    ret += "    init ();\n";
    ret += "    foo ();\n";
    ret += "    std::cout << checksum () << std::endl;\n";
    ret += "    return 0;\n";
    ret += "}";
    write_file("driver.cpp", ret);
    return ret;
}

