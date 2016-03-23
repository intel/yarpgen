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

#include "sym_table.h"

std::shared_ptr<Variable> SymbolTable::get_rand_variable () {
    int indx = rand_val_gen->get_rand_value<int>(0, variable.size() - 1);
    return variable.at(indx);
}

std::shared_ptr<Array> SymbolTable::get_rand_array () {
    int indx = rand_val_gen->get_rand_value<int>(0, array.size() - 1);
    return array.at(indx);
}

std::string SymbolTable::emit_variable_extern_decl () {
    std::string ret = "";
    for (auto i = variable.begin(); i != variable.end(); ++i) {
        DeclStmt decl;
        decl.set_data(*i);
        decl.set_is_extern(true);
        ret += decl.emit() + ";\n";
    }
    return ret;
}

std::string SymbolTable::emit_variable_def () {
    std::string ret = "";
    for (auto i = variable.begin(); i != variable.end(); ++i) {
        ConstExpr const_init;
        const_init.set_type (std::static_pointer_cast<IntegerType>((*i)->get_type())->get_int_type_id());
        const_init.set_data ((*i)->get_value());

        DeclStmt decl;
        decl.set_data(*i);
        decl.set_init (std::make_shared<ConstExpr>(const_init));
        decl.set_is_extern(false);
        ret += decl.emit() + ";\n";
    }
    return ret;
}

std::string SymbolTable::emit_variable_check () {
    std::string ret = "";
    for (auto i = variable.begin(); i != variable.end(); ++i) {
        ret += "hash(seed, " + (*i)->get_name() + ");\n";
    }
    return ret;
}

Context::Context (GenPolicy _gen_policy, std::shared_ptr<Stmt> _glob_stmt, Node::NodeID _glob_stmt_id,
                  std::shared_ptr<SymbolTable> _global_sym_table, std::shared_ptr<Context> _parent_ctx) {
    self_gen_policy = _gen_policy;
    self_glob_stmt = _glob_stmt;
    self_glob_stmt_id = _glob_stmt_id;
    global_sym_table = _global_sym_table;
    parent_ctx = _parent_ctx;
    depth = 0;
    if_depth = 0;

    if (parent_ctx != NULL) {
        extern_inp_sym_table = parent_ctx->get_extern_inp_sym_table ();
        extern_out_sym_table = parent_ctx->get_extern_out_sym_table ();
        depth = parent_ctx->get_depth();
        if_depth = parent_ctx->get_if_depth();
    }
    if (_glob_stmt_id == Node::NodeID::IF)
        if_depth++;
}

