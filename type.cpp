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

#include "type.h"

Type::Type (unsigned int _type_id) {
    this->id = _type_id;
    switch (_type_id) {
        case TypeID::UCHAR:
            this->name = "unsigned char";
            this->min_val = 0;
            this->max_val = UCHAR_MAX;
            this->is_fp = false;
            this->is_signed = false;
            break;
        case TypeID::USHRT:
            this->name = "unsigned short";
            this->min_val = 0;
            this->max_val = USHRT_MAX;
            this->is_fp = false;
            this->is_signed = false;
            break;
        case TypeID::UINT:
            this->name = "unsigned int";
            this->min_val = 0;
            this->max_val = UINT_MAX;
            this->is_fp = false;
            this->is_signed = false;
            break;
        case TypeID::ULINT:
            this->name = "unsigned long int";
            this->min_val = 0;
            this->max_val = ULONG_MAX;
            this->is_fp = false;
            this->is_signed = false;
            break;
         case TypeID::ULLINT:
            this->name = "unsigned long long int";
            this->min_val = 0;
            this->max_val = ULLONG_MAX;
            this->is_fp = false;
            this->is_signed = false;
            break;
    }
}

unsigned int Type::get_id () { return this->id; }

std::string Type::get_name () { return this->name; }

bool Type::get_is_fp () { return this->is_fp; }

bool Type::get_is_signed () { return this->is_signed; }

void Type::set_max_value (int64_t _max_val) { this->max_val = _max_val; }

int64_t Type::get_max_value () { return this->max_val; }

void Type::set_min_value (int64_t _min_val) { this->min_val = _min_val; }

int64_t Type::get_min_value () { return this->min_val; }

std::string Type::get_rand_value () { // TODO: can't handle fp types
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::string ret;
    if (this->name == "unsigned char") {
        std::uniform_int_distribution<unsigned char> dis(this->min_val, this->max_val);
        ret = std::to_string(dis(gen));
    }
    if (this->name == "unsigned short") {
        std::uniform_int_distribution<unsigned short> dis(this->min_val, this->max_val);
        ret = std::to_string(dis(gen));
    }
    if (this->name == "unsigned int") {
        std::uniform_int_distribution<unsigned int> dis(this->min_val, this->max_val);
        ret = std::to_string(dis(gen));
    }
    if (this->name == "unsigned long int") {
        std::uniform_int_distribution<unsigned long int> dis(this->min_val, this->max_val);
        ret = std::to_string(dis(gen));
    }
    if (this->name == "unsigned long long int") {
        std::uniform_int_distribution<unsigned long long int> dis(this->min_val, this->max_val);
        ret = std::to_string(dis(gen));
    }
    if (this->name == "unsigned int")
        ret += "U";
    if (this->name == "unsigned long int")
        ret += "UL";
    if (this->name == "unsigned long long int")
        ret += "ULL";
    return ret;
}

void Type::dbg_dump () {
    std::cout << "name " << this->name << "\nmin_val " << this->min_val << "\nmax_val " << this->max_val << "\nis_fp " << this->is_fp << "\nis_signed " << this->is_signed << std::endl;
}

