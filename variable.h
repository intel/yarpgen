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

#pragma once

#include "type.h"

namespace rl {

class Data {
    public:
        enum VarClassID {
            VAR, ARR, STRUCT, MAX_CLASS_ID
        };

        //TODO: Data can be not only integer, but who cares
        Data (std::string _name, std::shared_ptr<Type> _type, VarClassID _class_id) :
              type(_type), name(_name), class_id(_class_id) {}
        Type::Mod get_modifier () { return type->get_modifier(); }
        bool get_is_static () { return type->get_is_static(); }
        uint64_t get_align () { return type->get_align(); }
        VarClassID get_class_id () { return class_id; }
        std::string get_name () { return name; }
        std::shared_ptr<Type> get_type () { return type; }
        virtual void dbg_dump () = 0;

    protected:
        std::shared_ptr<Type> type;
        std::string name;

    private:
        VarClassID class_id;
};

class Struct : public Data {
    public:
        Struct (std::string _name, std::shared_ptr<StructType> _type) :
                Data(_name, _type, Data::VarClassID::STRUCT) { allocate_members(); }
        uint64_t get_num_of_members () { return members.size(); }
        std::shared_ptr<Data> get_member (unsigned int num);
        void dbg_dump ();

    private:
        void allocate_members();
        std::vector<std::shared_ptr<Data>> members;
};

class ScalarVariable : public Data {
    public:
        ScalarVariable (std::string _name, std::shared_ptr<IntegerType> _type);
        void set_init_value (AtomicType::ScalarTypedVal _init_val) { init_val = _init_val; was_changed = false; }
        void set_value (AtomicType::ScalarTypedVal _val) { cur_val = _val; was_changed = true; }
        void set_max (AtomicType::ScalarTypedVal _max) { max= _max; }
        void set_min (AtomicType::ScalarTypedVal _min) { min = _min; }
        AtomicType::ScalarTypedVal get_init_value () { return init_val; }
        AtomicType::ScalarTypedVal get_value () { return cur_val; }
        AtomicType::ScalarTypedVal get_max () { return max; }
        AtomicType::ScalarTypedVal get_min () { return min; }
        void dbg_dump ();

    private:
        AtomicType::ScalarTypedVal min;
        AtomicType::ScalarTypedVal max;
        AtomicType::ScalarTypedVal init_val;
        AtomicType::ScalarTypedVal cur_val;
        bool was_changed;
};

/*
class Array : public Data {
    public:
        enum Ess {
            C_ARR,
            STD_ARR,
            STD_VEC,
            VAL_ARR,
            MAX_ESS
        };

        explicit Array (std::string _name, IntegerType::IntegerTypeID _base_type_id,  Type::Mod _modifier, bool _is_static,
                        uint64_t _size, Ess _essence);
        std::shared_ptr<Type> get_base_type () { return base_type; }
        uint64_t get_size () { return size; }
        Ess get_essence () { return essence; }
        void set_value (uint64_t _val);
        void set_max (uint64_t _max);
        void set_min (uint64_t _min);
        uint64_t get_value ();
        uint64_t get_max ();
        uint64_t get_min ();
        void dbg_dump ();

    private:
        std::shared_ptr<Type> base_type;
        uint64_t size;
        Ess essence;
};
*/
}
