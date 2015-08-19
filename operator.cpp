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

#include "operator.h"

Operator::Operator () {
    this->id = MAX_OPER_TYPE;
    this->type [SELF] = Type::init (Type::TypeID::UCHAR);
    this->type [LEFT] = Type::init (Type::TypeID::UCHAR);
    this->type [RGHT] = Type::init (Type::TypeID::UCHAR);
    this->name = "";
    this->num_of_op = 0;
    this->cause_ub = false;
}

Operator::Operator (unsigned int _id) {
    this->id = _id;
    this->type [SELF] = Type::init (Type::TypeID::UCHAR);
    this->type [LEFT] = Type::init (Type::TypeID::UCHAR);
    this->type [RGHT] = Type::init (Type::TypeID::UCHAR);
    switch (_id) {
        case OperType::UN_INC:
            this->name = "+";
            this->num_of_op = 1;
            this->cause_ub = false;
            break;
        case OperType::UN_DEC:
            this->name = "-";
            this->num_of_op = 1;
            this->cause_ub = true;
            break;
        case OperType::LOG_NOT:
            this->name = "!";
            this->num_of_op = 1;
            this->cause_ub = false;
            break;
        case OperType::MUL:
            this->name = "*";
            this->num_of_op = 2;
            this->cause_ub = true;
            break;
        case OperType::DIV:
            this->name = "/";
            this->num_of_op = 2;
            this->cause_ub = false;
            break;
        case OperType::MOD:
            this->name = "%";
            this->num_of_op = 2;
            this->cause_ub = false;
            break;
        case OperType::ADD:
            this->name = "+";
            this->num_of_op = 2;
            this->cause_ub = true;
            break;
        case OperType::SUB:
            this->name = "-";
            this->num_of_op = 2;
            this->cause_ub = true;
            break;
        case OperType::BIT_SHL:
            this->name = "<<";
            this->num_of_op = 2;
            this->cause_ub = true;
            break;
        case OperType::BIT_SHR:
            this->name = ">>";
            this->num_of_op = 2;
            this->cause_ub = true;
            break;
        case OperType::MAX_OPER_TYPE:
            this->name = "ERROR in Operator";
            this->num_of_op = 0;
            this->cause_ub = true;
    }
}

Operator Operator::get_rand_obj () {
    std::uniform_int_distribution<unsigned int> dis(0, Operator::OperType::MAX_OPER_TYPE - 1);
    return Operator (dis(rand_gen));
}

void Operator::generate_domains () {
/*    if (!can_cause_ub())
        return;
    switch (get_id()) {
        case OperType::UN_INC:
            return;
        case OperType::UN_DEC:
            if (get_is_signed(SELF)) {
                // TODO
            }
            else {
                if (check_val_in_domains(RGHT, 0))
                    add_bound_value(RGHT, 0);
                set_min_value(RGHT,  
                    
            }
            break;
            
            
    };
*/
}

unsigned int Operator::get_id () const { return this->id; }

std::string Operator::get_name () const {  return this->name; }

unsigned int Operator::get_num_of_op () const { return this->num_of_op; }

void Operator::set_type (unsigned int side, std::shared_ptr<Type> _type) { this->type [side] = _type; }

std::shared_ptr<Type> Operator::get_type (unsigned int side) const {
    if (this->type [side] != NULL)
        return this->type [side];
    return NULL;
}

unsigned int Operator::get_type_id (unsigned int side) const {
    if (this->type [side] != NULL)
        return this->type [side]->get_id();
    return Type::TypeID::MAX_TYPE_ID;
}

std::string Operator::get_type_name (unsigned int side) const {
    if (this->type [side] != NULL)
        return this->type [side]->get_name();
    return "";
}

bool Operator::get_is_fp (unsigned int side) const {
    if (this->type [side] != NULL)
        return this->type [side]->get_is_fp();
    return true;
}

bool Operator::get_is_signed (unsigned int side) const {
    if (this->type [side] != NULL)
        return this->type [side]->get_is_signed();
    return true;
}

void Operator::set_max_value (unsigned int side, uint64_t _max_val) {
    if (this->type [side] != NULL)
        this->type [side]->set_max_value (_max_val);
}

uint64_t Operator::get_max_value (unsigned int side) const {
    if (this->type [side] != NULL)
        return this->type [side]->get_max_value ();
    return 0;
}

void Operator::set_min_value (unsigned int side, uint64_t _min_val) {
    if (this->type [side] != NULL)
        this->type [side]->set_min_value (_min_val);
}

uint64_t Operator::get_min_value (unsigned int side) const {
    if (this->type [side] != NULL)
        return this->type [side]->get_min_value ();
    return 0;
}

void Operator::set_bound_value (unsigned int side, std::vector<uint64_t> bval) { 
    if (this->type [side] != NULL)
        this->type [side]->set_bound_value (bval);
}

std::vector<uint64_t> Operator::get_bound_value (unsigned int side) const {
    std::vector<uint64_t> ret;
    if (this->type [side] != NULL)
        ret = this->type [side]->get_bound_value ();
    return ret;
}

void Operator::add_bound_value (unsigned int side, uint64_t bval) { 
    if (this->type [side] != NULL)
        this->type [side]->add_bound_value (bval); 
}

bool Operator::check_val_in_domains (unsigned int side, uint64_t val) { 
    if (this->type [side] != NULL)
        return this->type [side]->check_val_in_domains (val); 
    return false;
}

bool Operator::can_cause_ub () { return this->cause_ub; }

std::string Operator::emit_usage () {
    std::string ret = " " + this->get_name () + " ";
    return ret;
}

void Operator::dbg_dump () {
    std::cout << "id " << this->id << std::endl;
    std::cout << "name " << this->name << std::endl;
    std::cout << "num_of_op " << this->num_of_op << std::endl;
    std::cout << "self type " << std::endl;
    this->type [SELF]->dbg_dump();
    if (this->type [LEFT] != NULL) {
        std::cout << "left type " << std::endl;
        this->type [LEFT]->dbg_dump();
    }
    if (this->type [RGHT] != NULL) {
        std::cout << "right type " << std::endl;
        this->type [RGHT]->dbg_dump();
    }
    std::cout << "cause_ub " << this->cause_ub << std::endl;
}
