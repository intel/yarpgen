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
    this->type [SELF] = NULL;
    this->type [LEFT] = NULL;
    this->type [RGHT] = NULL;
    this->name = "";
    this->num_of_op = 0;
    this->cause_ub = false;
}

Operator::Operator (unsigned int _id) {
    this->id = _id;
    this->type [SELF] = new Type (Type::TypeID::UCHAR);
    this->type [LEFT] = NULL;
    this->type [RGHT] = NULL;
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

Operator::Operator (const Operator& _op) {
    this->id = _op.id;
    this->name = _op.name;
    this->num_of_op = _op.num_of_op;
    this->type [SELF] = new Type (_op.type [SELF]->get_id());
    this->type [SELF]->set_max_value (_op.type [SELF]->get_max_value());
    this->type [SELF]->set_min_value (_op.type [SELF]->get_min_value());
    this->cause_ub = _op.cause_ub;
}

Operator& Operator::operator=(const Operator& _op) {
    if (this != &_op) {
        this->id = _op.id;
        this->name = _op.name;
        this->num_of_op = _op.num_of_op;
        delete this->type [SELF];
        this->type [SELF] = new Type (_op.type [SELF]->get_id());
        this->type [SELF]->set_max_value (_op.type [SELF]->get_max_value());
        this->type [SELF]->set_min_value (_op.type [SELF]->get_min_value());
        this->cause_ub = _op.cause_ub;
    }
    return *this;
}

Operator::~Operator () { delete this->type [SELF]; } 

unsigned int Operator::get_id () { return this->id; }

std::string Operator::get_name (){  return this->name; }

unsigned int Operator::get_num_of_op () { return this->num_of_op; }

void Operator::set_type (unsigned int side, Type* _type) {
    this->type [side] = _type;
}
 
Type* Operator::get_type (unsigned int side) {
    return this->type [side];
}

unsigned int Operator::get_type_id (unsigned int side) { 
    if (this->type [side] != NULL)
        return this->type [side]->get_id(); 
    return Type::TypeID::MAX_TYPE_ID;
}

std::string Operator::get_type_name (unsigned int side) { 
    if (this->type [side] != NULL)
        return this->type [side]->get_name(); 
    return "";
}

bool Operator::get_is_fp (unsigned int side) { 
    if (this->type [side] != NULL)
        return this->type [side]->get_is_fp(); 
    return true;
}

bool Operator::get_is_signed (unsigned int side) { 
    if (this->type [side] != NULL)
        return this->type [side]->get_is_signed(); 
    return true;
}

void Operator::set_max_value (unsigned int side, int64_t _max_val) { 
    if (this->type [side] != NULL)
        this->type [side]->set_max_value (_max_val); 
}

int64_t Operator::get_max_value (unsigned int side) { 
    if (this->type [side] != NULL)
        return this->type [side]->get_max_value (); 
    return 0;
}

void Operator::set_min_value (unsigned int side, int64_t _min_val) { 
    if (this->type [side] != NULL)
        this->type [side]->set_min_value (_min_val); 
}

int64_t Operator::get_min_value (unsigned int side) { 
    if (this->type [side] != NULL)
        return this->type [side]->get_min_value (); 
    return 0;
}

bool Operator::can_cause_ub () { return this->cause_ub; }

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
