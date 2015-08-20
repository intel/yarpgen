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

#include "tree_elem.h"

TreeElem::TreeElem (bool _is_op, std::shared_ptr<Array> _array, unsigned int _oper_type_id) {
    this->is_op = _is_op;
    if (this->is_op) {
        this->oper = Operator (_oper_type_id);
        this->array = NULL;
    }
    else {
        this->array = _array;
        this->oper = Operator (Operator::OperType::MAX_OPER_TYPE);
    }
}

TreeElem TreeElem::get_rand_obj_op (unsigned int _type_id) {
    std::uniform_int_distribution<unsigned int> dis(0, Operator::OperType::MAX_OPER_TYPE - 1);
    TreeElem ret = TreeElem (true, NULL, dis(rand_gen));
    ret.spread_oper_type (_type_id);
    return ret;
}

TreeElem TreeElem::get_rand_obj_op (std::shared_ptr<Type> _type) {
    TreeElem ret = TreeElem::get_rand_obj_op (Type::TypeID::UCHAR);
    ret.spread_oper_type (_type);
    return ret;
}

void TreeElem::determine_range () {
       // TODO: replace everything
    spread_oper_type (get_oper_type(Operator::Side::SELF));
    set_oper_max_value(Operator::Side::LEFT, get_oper_max_value(Operator::Side::SELF) / 2);
    set_oper_max_value(Operator::Side::RGHT, get_oper_max_value(Operator::Side::SELF) / 2);
    set_oper_min_value(Operator::Side::LEFT, get_oper_min_value(Operator::Side::SELF) / 2);
    set_oper_min_value(Operator::Side::RGHT, get_oper_min_value(Operator::Side::SELF) / 2);
}

bool TreeElem::get_is_op () { return this->is_op; }

std::string TreeElem::get_arr_name () { 
    if (this->array != NULL)
        return this->array->get_name();
    return "";
}

unsigned int TreeElem::get_arr_size () {
    if (this->array != NULL)
        return this->array->get_size();
    return 0;
}

std::string TreeElem::get_arr_type_name () {
    if (this->array != NULL)
        return this->array->get_type_name();
    return 0; 
}

void TreeElem::set_arr_max_value (int64_t _max_val) {
    if (this->array != NULL)
        this->array->set_max_value(_max_val);
}

int64_t TreeElem::get_arr_max_value () {
    if (this->array != NULL)
        return this->array->get_max_value();
    return 0;
}

void TreeElem::set_arr_min_value (int64_t _min_val) {
    if (this->array != NULL)
        this->array->set_min_value(_min_val);
}

int64_t TreeElem::get_arr_min_value () {
    if (this->array != NULL)
        return this->array->get_min_value();
    return 0;
}

bool TreeElem::get_arr_is_fp () {
    if (this->array != NULL)
        return this->array->get_is_fp();
    return false;
}

bool TreeElem::get_arr_is_signed () {
    if (this->array != NULL)
        return this->array->get_is_signed();
    return false;
}

std::shared_ptr<Array> TreeElem::get_array () { return this->array; }

unsigned int TreeElem::get_oper_id () { return this->oper.get_id(); }

std::string TreeElem::get_oper_name () { return this->oper.get_name(); }

unsigned int TreeElem::get_num_of_op () { return this->oper.get_num_of_op(); }

bool TreeElem::can_oper_cause_ub () { return this->oper.can_cause_ub(); }

void TreeElem::set_oper_type (unsigned int side, std::shared_ptr<Type> _type) { this->oper.set_type (side, _type); }

void TreeElem::set_oper_type (unsigned int side, unsigned int _type_id) { this->oper.set_type (side, Type::init(_type_id)); }

void TreeElem::spread_oper_type (std::shared_ptr<Type> _type) { this->oper.spread_type (_type); }

void TreeElem::spread_oper_type (unsigned int _type_id) { this->oper.spread_type (Type::init(_type_id)); }

std::shared_ptr<Type> TreeElem::get_oper_type (unsigned int side) { return this->oper.get_type (side); }

unsigned int TreeElem::get_oper_type_id (unsigned int side) { return this->oper.get_type_id (side); }

std::string TreeElem::get_oper_type_name (unsigned int side) { return this->oper.get_type_name (side); }

bool TreeElem::get_oper_type_is_fp (unsigned int side) { return this->oper.get_is_fp (side); }

bool TreeElem::get_oper_type_is_signed (unsigned int side) { return this->oper.get_is_signed (side); }

void TreeElem::set_oper_max_value (unsigned int side, int64_t _max_val) { this->oper.set_max_value (side, _max_val); }

int64_t TreeElem::get_oper_max_value (unsigned int side) { return this->oper.get_max_value (side); }

void TreeElem::set_oper_min_value (unsigned int side, int64_t _min_val) { this->oper.set_min_value (side, _min_val); }

int64_t TreeElem::get_oper_min_value (unsigned int side) { return this->oper.get_min_value (side); }

Operator TreeElem::get_oper () { return this->oper; }

std::string TreeElem::get_type_name () {
    if (get_is_op())
        return get_oper_type_name(Operator::Side::SELF);
    else 
        return get_arr_type_name(); 
}

uint64_t TreeElem::get_max_value () {
    if (get_is_op())
        return get_oper_max_value(Operator::Side::SELF);
    else
        return get_arr_max_value();
}

uint64_t TreeElem::get_min_value () {
    if (get_is_op())
        return get_oper_min_value(Operator::Side::SELF);
    else
        return get_arr_min_value();
}

std::string TreeElem::emit_usage () {
    std::string ret;
    if (this->get_is_op () == false && 
        this->get_oper_id () == Operator::OperType::MAX_OPER_TYPE && 
        this->get_array () != NULL)
        ret = this->array->emit_usage ();
    else
        ret = this->oper.emit_usage ();
    return ret;
}

void TreeElem::dbg_dump () {
    std::cout << "is_op " << this->is_op << std::endl;
    if (this->array != NULL) this->array->dbg_dump();
    if (this->oper.get_id() != Operator::OperType::MAX_OPER_TYPE) this->oper.dbg_dump();
}
