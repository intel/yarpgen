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

#include "generator.h"

void ScalarTypeGen::generate () {
    type_id = (Type::TypeID) rand_val_gen->get_rand_id(gen_policy->get_allowed_types());
}

void ModifierGen::generate () {
    modifier = gen_policy->get_allowed_modifiers().at(
               rand_val_gen->get_rand_value<int>(0, gen_policy->get_allowed_modifiers().size() - 1));
}

void StaticSpecifierGen::generate () {
    if (gen_policy->get_allow_static_var())
        specifier = rand_val_gen->get_rand_value<int>(0, 1);
    else
        specifier = false;
}

void VariableValueGen::generate () {
    if (rand_init) {
        std::shared_ptr<Type> tmp_type = Type::init (type_id);
        min_value = tmp_type->get_min ();
        max_value = tmp_type->get_max ();
    }
    switch (type_id) {
        case Type::TypeID::BOOL:
            {
                value = (bool) rand_val_gen->get_rand_value<int>((bool) min_value, (bool) max_value);
            }
            break;
        case Type::TypeID::CHAR:
            {
                value = (signed char) rand_val_gen->get_rand_value<signed char>((signed char) min_value,
                                                                                (signed char) max_value);
            }
            break;
        case Type::TypeID::UCHAR:
            {
                value = (unsigned char) rand_val_gen->get_rand_value<unsigned char>((unsigned char) min_value,
                                                                                    (unsigned char) max_value);
            }
            break;
        case Type::TypeID::SHRT:
            {
                value = (short) rand_val_gen->get_rand_value<short>((short) min_value,
                                                                  (short) max_value);
            }
            break;
        case Type::TypeID::USHRT:
            {
                value = (unsigned short) rand_val_gen->get_rand_value<unsigned short>((unsigned short) min_value,
                                                                                      (unsigned short) max_value);
            }
            break;
        case Type::TypeID::INT:
            {
                value = (int) rand_val_gen->get_rand_value<int>((int) min_value,
                                                              (int) max_value);
            }
            break;
        case Type::TypeID::UINT:
            {
                value = (unsigned int) rand_val_gen->get_rand_value<unsigned int>((unsigned int) min_value,
                                                                                  (unsigned int) max_value);
            }
            break;
        case Type::TypeID::LINT:
            {
                value = (long int) rand_val_gen->get_rand_value<long int>((long int) min_value,
                                                                          (long int) max_value);
            }
            break;
        case Type::TypeID::ULINT:
            {
                value = (unsigned long int) rand_val_gen->get_rand_value<unsigned long int>((unsigned long int) min_value,
                                                                                            (unsigned long int) max_value);
            }
            break;
        case Type::TypeID::LLINT:
            {
                value = (long long int) rand_val_gen->get_rand_value<long long int>((long long int) min_value,
                                                                                    (long long int) max_value);
            }
            break;
        case Type::TypeID::ULLINT:
            {
                value = (unsigned long long int) rand_val_gen->get_rand_value<unsigned long long int>((unsigned long long int) min_value,
                                                                                                      (unsigned long long int) max_value);
            }
            break;
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad type in VariableValueGen::generate." << std::endl;
            exit (-1);
            break;
    }
}

void DataGen::rand_init_param () {
    if (!rand_init)
        return;

    ScalarTypeGen scalar_type_gen (gen_policy);
    scalar_type_gen.generate ();
    type_id = scalar_type_gen.get_type ();

    ModifierGen modifier_gen (gen_policy);
    modifier_gen.generate ();
    modifier = modifier_gen.get_modifier ();

    StaticSpecifierGen static_spec_gen (gen_policy);
    static_spec_gen.generate ();
    static_spec = static_spec_gen.get_specifier ();
}

void DataGen::rand_init_value () {
    VariableValueGen var_val_gen (gen_policy, type_id);
    var_val_gen.generate();
    assert (data != NULL && "DataGen::rand_init_value with empty data.");
    data->set_value(var_val_gen.get_value());
}

int ScalarVariableGen::variable_num = 0;

void ScalarVariableGen::generate () {
    rand_init_param ();
    Variable tmp_var ("var_" + std::to_string(variable_num), type_id, modifier, static_spec);
    data = std::make_shared<Variable> (tmp_var);
    rand_init_value();
    variable_num++;
}

int ArrayVariableGen::array_num = 0;

void ArrayVariableGen::generate () {
    rand_init_param ();
    if (rand_init) {
        size = rand_val_gen->get_rand_value<int>(gen_policy->get_min_array_size(), gen_policy->get_max_array_size());
        essence = (Array::Ess) (gen_policy->get_essence_differ() ? rand_val_gen->get_rand_value<int>(0, Array::Ess::MAX_ESS - 1) :
                                                                   gen_policy->get_primary_essence());
    }
    Array tmp_arr ("arr_" + std::to_string(array_num), type_id, modifier, static_spec, size, essence);
    data = std::make_shared<Array> (tmp_arr);
    rand_init_value();
    array_num++;
}

void DeclStmtGen::generate () {
    // TODO: Add non-scalar variables declaration
    if (rand_init) {
        // TODO: Enable gen_policy limits
        if (var_class_id == Data::VarClassID::VAR) {
            ScalarVariableGen scalar_var_gen (gen_policy);
            scalar_var_gen.generate ();
            data = scalar_var_gen.get_data();
        }
        else if (var_class_id == Data::VarClassID::ARR) {
            ArrayVariableGen array_var_gen (gen_policy);
            array_var_gen.generate ();
            data = array_var_gen.get_data ();
        }
        else {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": only scalar vasriables and arrays are allowed." << std::endl;
            exit (-1);
        }
        // TODO: Add auto-init generation
    }
    DeclStmt decl_stmt;
    decl_stmt.set_data (data);
    decl_stmt.set_init (init);
    decl_stmt.set_is_extern (is_extern);
    stmt = std::make_shared<DeclStmt> (decl_stmt);
}

void ArithStmtGen::generate () {
    res_expr = gen_level(0);
    if (out != NULL) {
        AssignExpr assign;
        assign.set_to(out);
        assign.set_from(res_expr);
        res_expr = std::make_shared<AssignExpr> (assign);
    }
    ExprStmt res_stmt;
    res_stmt.set_expr(res_expr);
    stmt = std::make_shared<ExprStmt> (res_stmt);
}

std::shared_ptr<Expr> ArithStmtGen::gen_level (int depth) {
    int node_type = rand_val_gen->get_rand_id (gen_policy->get_arith_leaves());
    if (node_type == GenPolicy::ArithLeafID::Data || depth == gen_policy->get_max_arith_depth()) {
        int data_type = rand_val_gen->get_rand_id (gen_policy->get_arith_data_distr());
        if (data_type == GenPolicy::ArithDataID::Inp) {
            int inp_use = rand_val_gen->get_rand_value<int>(0, inp.size() - 1);
            inp.at(inp_use)->propagate_type();
            inp.at(inp_use)->propagate_value();
            return inp.at(inp_use);
        }
        else if (data_type == GenPolicy::ArithDataID::Const) {
            Type::TypeID type_id = (Type::TypeID) rand_val_gen->get_rand_id(gen_policy->get_allowed_types());
            VariableValueGen var_val_gen (gen_policy, type_id);
            var_val_gen.generate ();
            ConstExpr const_expr;
            const_expr.set_type (type_id);;
            const_expr.set_data (var_val_gen.get_value());
            const_expr.propagate_type();
            const_expr.propagate_value();
            return std::make_shared<ConstExpr> (const_expr);
        }
        else {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": inappropriate data type." << std::endl;
            exit (-1);
        }
    }
    else if (node_type == GenPolicy::ArithLeafID::Unary) { // Unary expr
        UnaryExpr unary;
        UnaryExpr::Op op_type = (UnaryExpr::Op) rand_val_gen->get_rand_id(gen_policy->get_allowed_unary_op());
        unary.set_op(op_type);
        unary.set_arg(gen_level(depth + 1));
        unary.propagate_type();
        Expr::UB ub = unary.propagate_value();
        std::shared_ptr<Expr> ret = std::make_shared<UnaryExpr> (unary);
        if (ub)
            ret = rebuild_unary(ub, ret);
        return ret;
    }
    else if (node_type == GenPolicy::ArithLeafID::Binary) {// Binary expr
        BinaryExpr binary;
        BinaryExpr::Op op_type = (BinaryExpr::Op) rand_val_gen->get_rand_id(gen_policy->get_allowed_binary_op());
        binary.set_op(op_type);
        binary.set_lhs(gen_level(depth + 1));
        binary.set_rhs(gen_level(depth + 1));
        binary.propagate_type();
        Expr::UB ub = binary.propagate_value();
        std::shared_ptr<Expr> ret = std::make_shared<BinaryExpr> (binary);
        if (ub)
            ret = rebuild_binary(ub, ret);
        return ret;
    }
    else if (node_type == GenPolicy::ArithLeafID::TypeCast) {// TypeCast expr
        TypeCastExpr type_cast;
        Type::TypeID type_id = (Type::TypeID) rand_val_gen->get_rand_id(gen_policy->get_allowed_types());
        type_cast.set_type(Type::init(type_id));
        type_cast.set_expr(gen_level(depth + 1));
        type_cast.propagate_type();
        type_cast.propagate_value();
        return std::make_shared<TypeCastExpr> (type_cast);
    }
    else {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": inappropriate node type." << std::endl;
        exit (-1);
    }
    return NULL;
}

std::shared_ptr<Expr> ArithStmtGen::rebuild_unary (Expr::UB ub, std::shared_ptr<Expr> expr) {
    if (expr->get_id() != Node::NodeID::UNARY) {
        std::cerr << "ERROR: ArithExprGen::rebuild_unary : not UnaryExpr" << std::endl;
        return NULL;
    }
    std::shared_ptr<UnaryExpr> ret = std::static_pointer_cast<UnaryExpr>(expr);
    switch (ret->get_op()) {
        case UnaryExpr::PreInc:
            ret->set_op(UnaryExpr::Op::PreDec);
            break;
        case UnaryExpr::PostInc:
            ret->set_op(UnaryExpr::Op::PostDec);
            break;
        case UnaryExpr::PreDec:
            ret->set_op(UnaryExpr::Op::PreInc);
            break;
        case UnaryExpr::PostDec:
            ret->set_op(UnaryExpr::Op::PostInc);
            break;
        case UnaryExpr::Negate:
            ret->set_op(UnaryExpr::Op::Plus);
            break;
        case UnaryExpr::Plus:
        case UnaryExpr::LogNot:
        case UnaryExpr::BitNot:
            break;
        case UnaryExpr::MaxOp:
            std::cerr << "ERROR: ArithExprGen::rebuild_unary : MaxOp" << std::endl;
            break;
    }
    ret->propagate_type();
    Expr::UB ret_ub = ret->propagate_value();
    if (ret_ub) {
        std::cerr << "ERROR: ArithExprGen::rebuild_unary : illegal strategy" << std::endl;
        return NULL;
    }
    return ret;
}

static uint64_t msb(uint64_t x) {
    uint64_t ret = 0;
    while (x != 0) {
        ret++;
        x = x >> 1;
    }
    return ret;
}

std::shared_ptr<Expr> ArithStmtGen::rebuild_binary (Expr::UB ub, std::shared_ptr<Expr> expr) {
    if (expr->get_id() != Node::NodeID::BINARY) {
        std::cerr << "ERROR: ArithExprGen::rebuild_binary : not BinaryExpr" << std::endl;
        return NULL;
    }
    std::shared_ptr<BinaryExpr> ret = std::static_pointer_cast<BinaryExpr>(expr);
    switch (ret->get_op()) {
        case BinaryExpr::Add:
            ret->set_op(BinaryExpr::Op::Sub);
            break;
        case BinaryExpr::Sub:
            ret->set_op(BinaryExpr::Op::Add);
            break;
        case BinaryExpr::Mul:
            if (ub == Expr::SignOvfMin)
                ret->set_op(BinaryExpr::Op::Sub);
            else
                ret->set_op(BinaryExpr::Op::Div);
            break;
        case BinaryExpr::Div:
        case BinaryExpr::Mod:
            if (ub == Expr::ZeroDiv)
                ret->set_op(BinaryExpr::Op::Mul);
            else
               ret->set_op(BinaryExpr::Op::Sub);
            break;
        case BinaryExpr::Shr:
        case BinaryExpr::Shl:
            if ((ub == Expr::ShiftRhsNeg) || (ub == Expr::ShiftRhsLarge)) {
                std::shared_ptr<Expr> rhs = ret->get_rhs();
                ConstExpr const_expr;
                const_expr.set_type(rhs->get_type_id());

                std::shared_ptr<Expr> lhs = ret->get_lhs();
                uint64_t max_sht_val = lhs->get_type_bit_size();
                if ((ret->get_op() == BinaryExpr::Shl) && (lhs->get_type_is_signed()) && (ub == Expr::ShiftRhsLarge))
                    max_sht_val = max_sht_val - msb((uint64_t)lhs->get_value());
                uint64_t const_val = rand_val_gen->get_rand_value<int>(0, max_sht_val);
                if (ub == Expr::ShiftRhsNeg) {
                    std::shared_ptr<Type> tmp_type = Type::init (rhs->get_type_id());
                    const_val += std::abs((long long int) rhs->get_value());
                    const_val = std::min(const_val, tmp_type->get_max()); // TODO: it won't work with INT_MIN
                }
                else
                    const_val = rhs->get_value() - const_val;
                const_expr.set_data(const_val);
                const_expr.propagate_type();
                const_expr.propagate_value();

                BinaryExpr ins;
                if (ub == Expr::ShiftRhsNeg)
                    ins.set_op(BinaryExpr::Op::Add);
                else
                    ins.set_op(BinaryExpr::Op::Sub);
                ins.set_lhs(rhs);
                ins.set_rhs(std::make_shared<ConstExpr> (const_expr));
                ins.propagate_type();
                Expr::UB ins_ub = ins.propagate_value();
                ret->set_rhs(std::make_shared<BinaryExpr> (ins));
                if (ins_ub) {
                    std::cerr << "ArithExprGen::rebuild_binary : invalid shift rebuild (type 1)" << std::endl;
                    ret = std::static_pointer_cast<BinaryExpr> (rebuild_binary(ins_ub, ret));
                }
            }
            else {
                std::shared_ptr<Expr> lhs = ret->get_lhs();
                ConstExpr const_expr;
                const_expr.set_type(lhs->get_type_id());
                uint64_t const_val = Type::init(lhs->get_type_id())->get_max();
                const_expr.set_data(const_val);
                const_expr.propagate_type();
                const_expr.propagate_value();

                BinaryExpr ins;
                ins.set_op(BinaryExpr::Op::Add);
                ins.set_lhs(lhs);
                ins.set_rhs(std::make_shared<ConstExpr> (const_expr));
                ins.propagate_type();
                Expr::UB ins_ub = ins.propagate_value();
                ret->set_lhs(std::make_shared<BinaryExpr> (ins));
                if (ins_ub) {
                    std::cerr << "ArithExprGen::rebuild_binary : invalid shift rebuild (type 2)" << std::endl;
                    ret = std::static_pointer_cast<BinaryExpr> (rebuild_binary(ins_ub, ret));
                }
            }
            break;
        case BinaryExpr::Lt:
        case BinaryExpr::Gt:
        case BinaryExpr::Le:
        case BinaryExpr::Ge:
        case BinaryExpr::Eq:
        case BinaryExpr::Ne:
        case BinaryExpr::BitAnd:
        case BinaryExpr::BitOr:
        case BinaryExpr::BitXor:
        case BinaryExpr::LogAnd:
        case BinaryExpr::LogOr:
            break;
        case BinaryExpr::MaxOp:
            std::cerr << "ArithExprGen::rebuild_binary : invalid Op" << std::endl;
            break;
    }
    ret->propagate_type();
    Expr::UB ret_ub = ret->propagate_value();
    if (ret_ub) {
        return rebuild_binary(ret_ub, ret);
    }
    return ret;
}

void ScopeGen::generate () {
    Node::NodeID stmt_id = Node::NodeID::EXPR;
    std::vector<std::shared_ptr<Expr>> inp;
    for (auto i = ctx->get_extern_inp_sym_table()->get_variables().begin(); i != ctx->get_extern_inp_sym_table()->get_variables().end(); ++i) {
        VarUseExpr var_use;
        var_use.set_variable (*i);
        inp.push_back(std::make_shared<VarUseExpr> (var_use));
    }
    VarUseExpr var_use;
    var_use.set_variable (ctx->get_extern_out_sym_table()->get_variables().at(0));
    Context arith_ctx (*(gen_policy), NULL, Node::NodeID::MAX_STMT_ID, std::make_shared<SymbolTable>(local_sym_table), ctx);
    ArithStmtGen arith_stmt_gen (std::make_shared<Context>(arith_ctx), inp, std::make_shared<VarUseExpr> (var_use));
    arith_stmt_gen.generate();
    scope.push_back(arith_stmt_gen.get_stmt());
}
