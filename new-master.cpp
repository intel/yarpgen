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
}

void Master::generate () {
    int var_num = rand_val_gen->get_rand_value<int>(5, 10);
    for (int i = 0; i < var_num; ++i) {
        ScalarVariableGen scalar_var_gen (std::make_shared<GenPolicy>(gen_policy));
        scalar_var_gen.generate ();
        extern_inp_sym_table.add_variable (std::static_pointer_cast<Variable>(scalar_var_gen.get_data()));
    }
    ScalarVariableGen scalar_var_gen (std::make_shared<GenPolicy>(gen_policy));
    scalar_var_gen.generate ();
    extern_out_sym_table.add_variable (std::static_pointer_cast<Variable>(scalar_var_gen.get_data()));

    Context ctx (gen_policy, NULL, Node::NodeID::MAX_STMT_ID, NULL, NULL);
    ctx.set_extern_inp_sym_table (std::make_shared<SymbolTable>(extern_inp_sym_table));
    ctx.set_extern_out_sym_table (std::make_shared<SymbolTable>(extern_out_sym_table));

    ScopeGen scope_gen (std::make_shared<Context> (ctx));
    scope_gen.generate();
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

    ret += extern_inp_sym_table.emit_variable_def();
    ret += extern_out_sym_table.emit_variable_def();

    ret += "void init () {\n";
    for (auto i = program.begin(); i != program.end(); ++i)
        ret += (*i)->emit() + ";\n";
    ret += "}";

    write_file("init.cpp", ret);
    return ret;
}
