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
#include "gen_policy.h"
#include "variable.h"

///////////////////////////////////////////////////////////////////////////////

class Generator {
    public:
        Generator (std::shared_ptr<GenPolicy> _gen_policy) : gen_policy (_gen_policy) {};
        virtual void generate () = 0;

    protected:
        std::shared_ptr<GenPolicy> gen_policy;
};

class ScalarTypeGen : public Generator {
    public:
        ScalarTypeGen (std::shared_ptr<GenPolicy> _gen_policy) : Generator (_gen_policy), type (NULL) {};
        std::shared_ptr<Type> get_type() { return type; }
        void generate ();

    private:
        std::shared_ptr<Type> type;
};

class ModifierGen : public Generator {
    public:
        ModifierGen (std::shared_ptr<GenPolicy> _gen_policy) : Generator (_gen_policy), modifier (Data::Mod::MAX_MOD) {}
        Data::Mod get_modifier () { return modifier; }
        void generate ();

    private:
        Data::Mod modifier;
};
