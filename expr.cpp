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
        case Data::VarClassID::VAR: {
            std::shared_ptr<ScalarVariable> scalar_var = std::make_shared<ScalarVariable>(*(std::static_pointer_cast<ScalarVariable>(value)));
            scalar_var->set_name("");
            return scalar_var;
        }
        case Data::VarClassID::STRUCT: {
            std::shared_ptr<Struct> struct_var = std::make_shared<Struct>(*(std::static_pointer_cast<Struct>(value)));
            struct_var->set_name("");
            return struct_var;
        }
        case Data::VarClassID::MAX_CLASS_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported Data::VarClassID in Expr::get_value" << std::endl;
            exit(-1);
    }
}

void VarUseExpr::set_value (std::shared_ptr<Data> _new_value) {
    if (_new_value->get_class_id() != value->get_class_id()) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": different Data::VarClassID in VarUseExpr::set_value" << std::endl;
        exit(-1);
    }
    switch (value->get_class_id()) {
        case Data::VarClassID::VAR:
            //TODO: Add integer type id check. We can't assign different types
            std::static_pointer_cast<ScalarVariable>(value)->set_cur_value(std::static_pointer_cast<ScalarVariable>(_new_value)->get_cur_value());
            break;
        case Data::VarClassID::STRUCT:
            //TODO: implement for Struct
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": Struct is unsupported in in VarUseExpr::set_value" << std::endl;
            exit(-1);
            break;
        case Data::VarClassID::MAX_CLASS_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported Data::VarClassID in VarUseExpr::set_value" << std::endl;
            exit(-1);
    }
}

AssignExpr::AssignExpr (std::shared_ptr<Expr> _to, std::shared_ptr<Expr> _from) :
                        Expr(Node::NodeID::ASSIGN, _to->get_value()), to(_to), from(_from) {
    if (to->get_id() != Node::NodeID::VAR_USE) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can assign only to variable in AssignExpr::AssignExpr" << std::endl;
        exit(-1);
    }
    propagate_type();
    propagate_value();
}

bool AssignExpr::propagate_type () {
    //TODO:StructType check for struct assignment
        if (value->get_class_id() == Data::VarClassID::VAR &&
            from->get_value()->get_class_id() == Data::VarClassID::VAR) {
            from = std::make_shared<TypeCastExpr>(from, value->get_type());
        }
        else {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": struct are unsupported in AssignExpr::propagate_value" << std::endl;
            exit(-1);
        }
    return true;
}

Expr::UB AssignExpr::propagate_value () {
    if (to->get_id() == Node::NodeID::VAR_USE) {
            std::static_pointer_cast<VarUseExpr>(to)->set_value(from->get_value());
    }
    else {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can assign only to variable in AssignExpr::propagate_value" << std::endl;
        exit(-1);
    }
    value = from->get_value();
    return NoUB;
}

std::string AssignExpr::emit (std::string offset) {
    std::string ret = offset;
    ret += to->emit();
    ret += " = ";
    ret += from->emit();
    return ret;
}

TypeCastExpr::TypeCastExpr (std::shared_ptr<Expr> _expr, std::shared_ptr<Type> _type) :
              Expr(Node::NodeID::VAR_USE, NULL), expr(_expr), to_type(_type) {
    propagate_type();
    propagate_value();
}

bool TypeCastExpr::propagate_type () {
    if (to_type->get_int_type_id() == Type::IntegerTypeID::MAX_INT_ID ||
        expr->get_value()->get_type()->get_int_type_id() == Type::IntegerTypeID::MAX_INT_ID) {
        //TODO: what about overloaded struct types cast?
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can cast only integer types in TypeCastExpr::propagate_type" << std::endl;
        exit(-1);
    }
    return true;
}

Expr::UB TypeCastExpr::propagate_value () {
    if (expr->get_value()->get_class_id() != Data::VarClassID::VAR) {
        //TODO: what about overloaded struct types cast?
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can cast only integer types in TypeCastExpr::propagate_value" << std::endl;
        exit(-1);
    }
    //TODO: Is it always safe to cast value to ScalarVariable?
    value = std::make_shared<ScalarVariable>("", std::static_pointer_cast<IntegerType>(to_type));
    std::static_pointer_cast<ScalarVariable>(value)->set_cur_value(std::static_pointer_cast<ScalarVariable>(expr->get_value())->get_cur_value().cast_type(to_type->get_int_type_id()));
    return NoUB;
}

std::string TypeCastExpr::emit (std::string offset) {
    std::string ret = offset;
    ret += "(" + value->get_type()->get_simple_name() + ")";
    ret += "(" + expr->emit() + ")";
    return ret;
}
