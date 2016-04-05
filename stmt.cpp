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

#include "stmt.h"

using namespace rl;

std::string DeclStmt::emit (std::string offset) {
    std::string ret = offset;
    ret += data->get_type()->get_is_static() && !is_extern ? "static " : "";
    ret += is_extern ? "extern " : "";
    switch (data->get_type()->get_modifier()) {
        case Type::Mod::VOLAT:
            ret += "volatile ";
            break;
        case Type::Mod::CONST:
            ret += "const ";
            break;
        case Type::Mod::CONST_VOLAT:
            ret += "const volatile ";
            break;
        case Type::Mod::NTHG:
            break;
        case Type::Mod::MAX_MOD:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad modifier in DeclStmt::emit" << std::endl;
            exit(-1);
            break;
    }
    ret += data->get_type()->get_simple_name() + " " + data->get_name();
    if (data->get_type()->get_align() != 0 && is_extern) // TODO: Should we set __attribute__ to non-extern variable?
        ret += " __attribute__((aligned(" + std::to_string(data->get_type()->get_align()) + ")))";
    if (init != NULL) {
        if (data->get_class_id() == Data::VarClassID::STRUCT) {
            std::cerr << "ERROR in DeclStmt::emit init of struct" << std::endl;
            exit(-1);
        }
        if (is_extern) {
            std::cerr << "ERROR in DeclStmt::emit init of extern" << std::endl;
            exit(-1);
        }
        ret += " = " + init->emit();
    }
    ret += ";";
    return ret;
}
