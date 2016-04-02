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

#include "type.h"
#include "variable.h"
#include "ir_node.h"
#include "expr.h"

using namespace rl;

std::shared_ptr<Data> Expr::get_value () {
    switch (value->get_class_id()) {
        case Data::VarClassID::VAR:
            return std::make_shared<ScalarVariable>(*(std::static_pointer_cast<ScalarVariable>(value)));
        case Data::VarClassID::STRUCT:
            return std::make_shared<Struct>(*(std::static_pointer_cast<Struct>(value)));
        case Data::VarClassID::MAX_CLASS_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported Data::VarClassID in Expr::get_value" << std::endl;
            exit(-1);
    }
}
