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

#pragma once

#include "type.h"

namespace yarpgen {

class Context;

class Data {
    public:
        enum VarClassID {
            VAR, STRUCT, MAX_CLASS_ID
        };

        //TODO: Data can be not only integer, but who cares
        Data (std::string _name, std::shared_ptr<Type> _type, VarClassID _class_id) :
              type(_type), name(_name), class_id(_class_id) {}
        VarClassID get_class_id () { return class_id; }
        std::string get_name () { return name; }
        void set_name (std::string _name) { name = _name; }
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
        uint64_t get_member_count () { return members.size(); }
        std::shared_ptr<Data> get_member (unsigned int num);
        void dbg_dump ();
        //TODO: stub for modifiers, cause now they are inside type.
        static std::shared_ptr<Struct> generate (std::shared_ptr<Context> ctx);
        static std::shared_ptr<Struct> generate (std::shared_ptr<Context> ctx, std::shared_ptr<StructType> struct_type);

    private:
        void allocate_members();
        void generate_members_init(std::shared_ptr<Context> ctx);
        std::vector<std::shared_ptr<Data>> members;
};

class ScalarVariable : public Data {
    public:
        ScalarVariable (std::string _name, std::shared_ptr<IntegerType> _type);
        //TODO: add check for type id in Type and Value
        void set_init_value (AtomicType::ScalarTypedVal _init_val) {init_val = cur_val = _init_val; was_changed = false; }
        void set_cur_value (AtomicType::ScalarTypedVal _val) { cur_val = _val; was_changed = true; }
        void set_max (AtomicType::ScalarTypedVal _max) { max = _max; }
        void set_min (AtomicType::ScalarTypedVal _min) { min = _min; }
        AtomicType::ScalarTypedVal get_init_value () { return init_val; }
        AtomicType::ScalarTypedVal get_cur_value () { return cur_val; }
        AtomicType::ScalarTypedVal get_max () { return max; }
        AtomicType::ScalarTypedVal get_min () { return min; }
        void dbg_dump ();
        static std::shared_ptr<ScalarVariable> generate(std::shared_ptr<Context> ctx);

    private:
        AtomicType::ScalarTypedVal min;
        AtomicType::ScalarTypedVal max;
        AtomicType::ScalarTypedVal init_val;
        AtomicType::ScalarTypedVal cur_val;
        bool was_changed;
};
}
