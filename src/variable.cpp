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

#include "sym_table.h"
#include "type.h"
#include "util.h"
#include "variable.h"

using namespace yarpgen;

void Struct::allocate_members() {
    std::shared_ptr<StructType> struct_type = std::static_pointer_cast<StructType>(type);
    // TODO: struct member can be not only integer
    for (int i = 0; i < struct_type->get_member_count(); ++i) {
        std::shared_ptr<StructType::StructMember> cur_member = struct_type->get_member(i);
        if (cur_member->get_type()->is_int_type()) {
            if (struct_type->get_member(i)->get_type()->get_is_static()) {
//                std::cout << struct_type->get_member(i)->get_definition() << std::endl;
//                std::cout << struct_type->get_member(i)->get_data().get() << std::endl;
                members.push_back(struct_type->get_member(i)->get_data());
            }
            else {
                std::shared_ptr<IntegerType> int_mem_type = std::static_pointer_cast<IntegerType> (struct_type->get_member(i)->get_type());
                ScalarVariable new_mem (struct_type->get_member(i)->get_name(), int_mem_type);
                members.push_back(std::make_shared<ScalarVariable>(new_mem));
            }
        }
        else if (cur_member->get_type()->is_struct_type()) {
            if (struct_type->get_member(i)->get_type()->get_is_static()) {
//                std::cout << struct_type->get_member(i)->get_definition() << std::endl;
//                std::cout << struct_type->get_member(i)->get_data().get() << std::endl;
                members.push_back(struct_type->get_member(i)->get_data());
            }
            else {
                Struct new_struct (cur_member->get_name(), std::static_pointer_cast<StructType>(cur_member->get_type()));
                members.push_back(std::make_shared<Struct>(new_struct));
            }
        }
        else {
            ERROR("unsupported type of struct member (Struct)");
        }
    }
}

std::shared_ptr<Data> Struct::get_member (unsigned int num) {
    if (num >= members.size())
        return nullptr;
    else
        return members.at(num);
}

void Struct::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump();
    std::cout << "name: " << name << std::endl;
    std::cout << "cv_qual: " << type->get_cv_qual() << std::endl;
    std::cout << "members ";
    for (auto i : members) {
        i->dbg_dump();
    }
}

std::shared_ptr<Struct> Struct::generate (std::shared_ptr<Context> ctx) {
    //TODO: what about nested structs? StructType::generate need it. Should it take it itself from context?
    std::shared_ptr<Struct> ret = std::make_shared<Struct>(rand_val_gen->get_struct_var_name(), StructType::generate(ctx));
    ret->generate_members_init(ctx);
    return ret;
}

std::shared_ptr<Struct> Struct::generate (std::shared_ptr<Context> ctx, std::shared_ptr<StructType> struct_type) {
    std::shared_ptr<Struct> ret = std::make_shared<Struct>(rand_val_gen->get_struct_var_name(), struct_type);
    ret->generate_members_init(ctx);
    return ret;
}

void Struct::generate_members_init(std::shared_ptr<Context> ctx) {
    for (int i = 0; i < get_member_count(); ++i) {
        if (get_member(i)->get_type()->is_struct_type()) {
            std::static_pointer_cast<Struct>(get_member(i))->generate_members_init(ctx);
        }
        else if (get_member(i)->get_type()->is_int_type()) {
            std::shared_ptr<IntegerType> member_type = std::static_pointer_cast<IntegerType>(get_member(i)->get_type());
            AtomicType::ScalarTypedVal init_val = AtomicType::ScalarTypedVal::generate(ctx, member_type->get_min(), member_type->get_max());
            std::static_pointer_cast<ScalarVariable>(get_member(i))->set_init_value(init_val);
        }
        else {
            ERROR("unsupported type of struct member (Struct)");
        }
    }
}

ScalarVariable::ScalarVariable (std::string _name, std::shared_ptr<IntegerType> _type) : Data (_name, _type, Data::VarClassID::VAR),
                    min(_type->get_int_type_id()), max(_type->get_int_type_id()),
                    init_val(_type->get_int_type_id()), cur_val(_type->get_int_type_id()) {
    min = _type->get_min();
    max = _type->get_max();
    init_val = _type->get_min();
    cur_val = _type->get_min();
    was_changed = false;
}

void ScalarVariable::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump();
    std::cout << "name: " << name << std::endl;
    std::cout << "cv_qual: " << type->get_cv_qual() << std::endl;
    std::cout << "init_value: " << init_val << std::endl;
    std::cout << "was_changed " << was_changed << std::endl;
    std::cout << "cur_value: " << cur_val << std::endl;
    std::cout << "min: " << min << std::endl;
    std::cout << "max: " << max << std::endl;
}

std::shared_ptr<ScalarVariable> ScalarVariable::generate(std::shared_ptr<Context> ctx) {
    std::shared_ptr<ScalarVariable> ret = std::make_shared<ScalarVariable> (rand_val_gen->get_scalar_var_name(), IntegerType::generate(ctx));
    ret->set_init_value(AtomicType::ScalarTypedVal::generate(ctx, ret->get_type()->get_int_type_id()));
    return ret;
}
