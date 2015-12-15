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

#include "master.h"

const int MIN_LOOP_NUM = 3;
const int MAX_LOOP_NUM = 10;

const int MIN_ARR_NUM = 3;
const int MAX_ARR_NUM = 10;

const int MIN_ARR_SIZE = 1000;
const int MAX_ARR_SIZE = 10000;

const int NUM_OF_MIXED_TYPES = 3;

const bool ESSENCE_DIFFER = false;

const int MAX_ARITH_DEPTH = 3;

uint64_t rand_dev () {
//    return 1646987221; // TODO: enable random
    std::random_device rd;
    uint64_t ret = rd ();
    std::cout << "/*SEED " << ret << "*/\n";
    return ret;
}

std::mt19937_64 rand_gen(rand_dev());

Master::Master (std::string _out_folder) {
    out_folder = _out_folder;

    ctrl.ext_num = "";

    int gen_types = 0;
    std::uniform_int_distribution<int> type_dis(Type::TypeID::CHAR, Type::TypeID::MAX_INT_ID - 1);
    while (gen_types < NUM_OF_MIXED_TYPES) {
        Type::TypeID type = (Type::TypeID) type_dis(rand_gen);
        if (std::find(ctrl.allowed_types.begin(), ctrl.allowed_types.end(), type) == ctrl.allowed_types.end()) {
            ctrl.allowed_types.push_back (type);
            gen_types++;
        }
    }

    ctrl.min_arr_num = MIN_ARR_NUM;
    ctrl.max_arr_num = MAX_ARR_NUM;
    ctrl.min_arr_size = MIN_ARR_SIZE;
    ctrl.max_arr_size = MAX_ARR_SIZE;

    std::uniform_int_distribution<int> ess_dis(0, Array::Ess::MAX_ESS - 1);
    ctrl.primary_ess = (Array::Ess) ess_dis (rand_gen);

    ctrl.min_var_val = 0;
    ctrl.max_var_val = UINT_MAX;

    ctrl.max_arith_depth = MAX_ARITH_DEPTH;

    ctrl.self_dep = true;
    ctrl.inter_war_dep = true;
}

void Master::generate () {
    // choose num of loops
    std::uniform_int_distribution<int> loop_num_dis(MIN_LOOP_NUM, MAX_LOOP_NUM);
    int loop_num = loop_num_dis(rand_gen);
    for (int i = 0; i < loop_num; ++i) {
        ctrl.ext_num = "lp" + std::to_string(i);
        LoopGen loop_gen (ctrl);
        loop_gen.generate();
        inp_sym_table.insert(inp_sym_table.end(), loop_gen.get_inp_sym_table().begin(), loop_gen.get_inp_sym_table().end());
        out_sym_table.insert(out_sym_table.end(), loop_gen.get_out_sym_table().begin(), loop_gen.get_out_sym_table().end());
        program.insert(program.end(), loop_gen.get_program().begin(), loop_gen.get_program().end());
    }
}

void LoopGen::generate () {
    // Array
    std::uniform_int_distribution<int> arr_num_dis(ctrl.min_arr_num, ctrl.max_arr_num);
    int arr_num = arr_num_dis(rand_gen);
    for (int i = 0; i < arr_num; ++i) {
        ControlStruct tmp_ctrl = ctrl;
        tmp_ctrl.ext_num += "_" + std::to_string(i);
        ArrayGen arr_gen (tmp_ctrl);
        arr_gen.generate ();

        std::shared_ptr<Array> inp_arr = arr_gen.get_inp_arr();
        min_ex_arr_size = std::min(min_ex_arr_size, inp_arr->get_size());
        inp_sym_table.push_back(inp_arr);
        sym_table.push_back(inp_arr);

        std::shared_ptr<Array> out_arr = arr_gen.get_out_arr();
        min_ex_arr_size = std::min(min_ex_arr_size, out_arr->get_size());
        out_sym_table.push_back(out_arr);
        sym_table.push_back(out_arr);
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
    // Loop
    CntLoopStmnt loop;
    std::uniform_int_distribution<int> loop_id_dis(0, LoopStmnt::LoopID::MAX_LOOP_ID - 1);
    LoopStmnt::LoopID loop_type = (LoopStmnt::LoopID) loop_id_dis(rand_gen);
    loop.set_loop_type (loop_type);

///////////////////////////////////////////////////////////////////////////////////////////////////
    // Trip
    TripGen trip_gen (ctrl, min_ex_arr_size);
    trip_gen.generate ();
    loop.set_iter(trip_gen.get_iter());
    loop.set_iter_decl(trip_gen.get_iter_decl());
    loop.set_step_expr(trip_gen.get_step_expr());
    loop.set_cond(trip_gen.get_cond());

///////////////////////////////////////////////////////////////////////////////////////////////////
    // Body
    std::vector<std::shared_ptr<Expr>> inp_expr;

    VarUseExpr iter_use;
    iter_use.set_variable(trip_gen.get_iter());


    for (auto i = inp_sym_table.begin(); i != inp_sym_table.end(); ++i) {
        IndexExpr arr_index;
        arr_index.set_index(std::make_shared<VarUseExpr> (iter_use));
        arr_index.set_base(std::static_pointer_cast<Array>(*i));
        inp_expr.push_back(std::make_shared<IndexExpr> (arr_index));
    }

    std::vector<std::shared_ptr<Expr>> out_expr;

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        IndexExpr out_arr_index;
        out_arr_index.set_index(std::make_shared<VarUseExpr> (iter_use));
        out_arr_index.set_base(std::static_pointer_cast<Array>(*i));
        out_expr.push_back(std::make_shared<IndexExpr> (out_arr_index));
    }

    std::vector<std::shared_ptr<Expr>> tmp_inp_expr;
    if (ctrl.inter_war_dep) {
        tmp_inp_expr = out_expr;
        tmp_inp_expr.insert(tmp_inp_expr.end(), inp_expr.begin(), inp_expr.end());
    }

    for (int i = 0; i < out_sym_table.size(); ++i) {
        if (ctrl.inter_war_dep) {
            if (!ctrl.self_dep) {
                tmp_inp_expr.erase(tmp_inp_expr.begin());
            }
        }
        else {
            tmp_inp_expr = inp_expr;
            tmp_inp_expr.push_back(out_expr.at(i));
        }

        ArithExprGen arith_expr_gen (ctrl, tmp_inp_expr, out_expr.at(i));
        arith_expr_gen.generate();
        loop.add_to_body(arith_expr_gen.get_expr_stmnt());

        if (ctrl.inter_war_dep) {
            tmp_inp_expr.erase(tmp_inp_expr.begin());
        }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
    program.push_back(std::make_shared<CntLoopStmnt> (loop));
}

void TripGen::generate () {
    std::uniform_int_distribution<int> var_type_dis(Type::TypeID::CHAR, Type::TypeID::MAX_INT_ID - 1);
    Type::TypeID var_type = (Type::TypeID) var_type_dis(rand_gen);
    Variable iter_var ("i_" + ctrl.ext_num, var_type, Variable::Mod::NTHNG, false);
    iter = std::make_shared<Variable> (iter_var);

    // Start value
    ControlStruct tmp_ctrl = ctrl;
    tmp_ctrl.min_var_val = 0;
    int64_t max_arr_indx = std::min(iter->get_max (), min_ex_arr_size - 1); // TODO: I hope it will work
    tmp_ctrl.max_var_val= max_arr_indx;
    VarValGen var_val_gen (tmp_ctrl, var_type);
    var_val_gen.generate();
    int64_t start_val = var_val_gen.get_value();
    iter->set_min(start_val);

    // Step
    std::uniform_int_distribution<int> step_dir_dis(0, 1);
    int step_dir = step_dir_dis(rand_gen) ? 1 : -1;
    if (start_val == max_arr_indx) {
        if (!iter->get_type()->get_is_signed()) { // If we have unsigned iterator, we can't go upper than top of array
            start_val = 0;
            iter->set_min(start_val);
        }
        else
            step_dir = -1;
    }
    if (start_val == 0)
        step_dir = 1;
    step_dir = iter->get_type()->get_is_signed() ? step_dir : 1;
    if (step_dir < 0 || start_val == max_arr_indx) { // Negative step
        tmp_ctrl.min_var_val = -1 * start_val;
        tmp_ctrl.max_var_val = -1;
    }
    if (step_dir > 0 || start_val == 0) { // Positive step
        tmp_ctrl.min_var_val = 1;
        tmp_ctrl.max_var_val = max_arr_indx - start_val;
    }
    var_val_gen = VarValGen (tmp_ctrl, Type::TypeID::LLINT);
    var_val_gen.generate_step();
    int64_t step = var_val_gen.get_value();
    iter->set_value(step);

    int64_t max_step_num = 0;
    int64_t a = 0;
    switch (var_type) {
        case Type::TypeID::CHAR:
            a = ((signed char) step_dir < 0) ? start_val : (max_arr_indx - start_val);
            max_step_num = std::abs((signed char) a / (signed char) step);
            break;
        case Type::TypeID::UCHAR:
            a = max_arr_indx - start_val;
            max_step_num = (unsigned char) a / (unsigned char) step;
            break;
        case Type::TypeID::SHRT:
            a = ((signed short) step_dir < 0) ? start_val : (max_arr_indx - start_val);
            max_step_num = std::abs((signed short) a / (signed short) step);
            break;
        case Type::TypeID::USHRT:
            a = max_arr_indx - start_val;
            max_step_num = (unsigned short) a / (unsigned short) step;
            break;
        case Type::TypeID::INT:
            a = ((signed int) step_dir < 0) ? start_val : (max_arr_indx - start_val);
            max_step_num = std::abs((signed int) a / (signed int) step);
            break;
        case Type::TypeID::UINT:
            a = max_arr_indx - start_val;
            max_step_num = (unsigned int) a / (unsigned int) step;
            break;
        case Type::TypeID::LINT:
            a = ((signed long int) step_dir < 0) ? start_val : (max_arr_indx - start_val);
            max_step_num = std::abs((signed long int) a / (signed long int) step);
            break;
        case Type::TypeID::ULINT:
            a = max_arr_indx - start_val;
            max_step_num = (unsigned long int) a / (unsigned long int) step;
            break;
        case Type::TypeID::LLINT:
            a = ((signed long long int) step_dir < 0) ? start_val : (max_arr_indx - start_val);
            max_step_num = std::abs((signed long long int) a / (signed long long int) step);
            break;
        case Type::TypeID::ULLINT:
            a = max_arr_indx - start_val;
            max_step_num = (unsigned long long int) a / (unsigned long long int) step;
            break;
        case Type::TypeID::BOOL:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "ERROR: TripGen::generate : bad iterator type" << std::endl;
            break;
    }
    std::uniform_int_distribution<int> step_num_dis(1, max_step_num);
    int64_t step_num = step_num_dis(rand_gen);

    // End value
    int64_t end_value = start_val + step_num * step;
    int64_t next_end_value = start_val + (step_num + 1) * step;
    bool can_iter_of = (next_end_value >= max_arr_indx) || (next_end_value < 0);
    std::uniform_int_distribution<int> end_diff_dis(0, 1);
    bool hit_end = end_diff_dis(rand_gen) || can_iter_of;
    end_value += hit_end ? 0 : step / 2;
    iter->set_max(end_value);

    StepExprGen step_expr_gen (ctrl, iter, step);
    step_expr_gen.generate();
    step_expr = step_expr_gen.get_expr();

    gen_condition(hit_end, step);
}

std::shared_ptr<DeclStmnt> TripGen::get_iter_decl () {
    ConstExpr iter_init;
    iter_init.set_type(iter->get_type()->get_id());
    iter_init.set_data(iter->get_min());
    DeclStmnt iter_decl;
    iter_decl.set_data(iter);
    iter_decl.set_init(std::make_shared<ConstExpr> (iter_init));
    return std::make_shared<DeclStmnt> (iter_decl);
}

void TripGen::gen_condition (bool hit_end, int64_t step) {
    std::vector<BinaryExpr::Op> allowed_cmp_op;
    if (hit_end)
        allowed_cmp_op.push_back(BinaryExpr::Op::Ne);
    if (step < 0) {
        if (!hit_end)
            allowed_cmp_op.push_back(BinaryExpr::Op::Ge);
        allowed_cmp_op.push_back(BinaryExpr::Op::Gt);
    }
    else {
        if (!hit_end)
            allowed_cmp_op.push_back(BinaryExpr::Op::Le);
        allowed_cmp_op.push_back(BinaryExpr::Op::Lt);
    }
    std::uniform_int_distribution<int> cmp_op_dis(0, allowed_cmp_op.size() - 1);
    BinaryExpr::Op cmp_op = allowed_cmp_op.at(cmp_op_dis(rand_gen));

    ConstExpr end_val_expr;
    end_val_expr.set_type(iter->get_type()->get_id());
    end_val_expr.set_data(iter->get_max());

    VarUseExpr iter_use;
    iter_use.set_variable(iter);

    BinaryExpr cond_expr;
    cond_expr.set_op(cmp_op);
    cond_expr.set_lhs(std::make_shared<VarUseExpr> (iter_use));
    cond_expr.set_rhs(std::make_shared<ConstExpr> (end_val_expr));
    cond = std::make_shared<BinaryExpr> (cond_expr);
}

void ArithExprGen::generate () {
    res_expr = gen_level(0);
    if (out == NULL)
        return;
    AssignExpr assign;
    assign.set_to(out);
    assign.set_from(res_expr);
    res_expr = std::make_shared<AssignExpr> (assign);
}

std::shared_ptr<Expr> ArithExprGen::gen_level (int depth) {
    std::uniform_int_distribution<int> node_type_dis(0, 100);
    int node_type = node_type_dis(rand_gen);
    if (node_type < 10 || depth == ctrl.max_arith_depth) { // Array
        std::uniform_int_distribution<int> inp_use_dis(0, inp.size() - 1);
        int inp_use = inp_use_dis(rand_gen);
        inp.at(inp_use)->propagate_type();
        inp.at(inp_use)->propagate_value();
        return inp.at(inp_use);
    }
    else if (node_type < 40) { // Unary expr
        UnaryExpr unary;
        std::uniform_int_distribution<int> op_type_dis(UnaryExpr::Op::Plus, UnaryExpr::Op::MaxOp - 1);
        UnaryExpr::Op op_type = (UnaryExpr::Op) op_type_dis(rand_gen);
        unary.set_op(op_type);
        unary.set_arg(gen_level(depth + 1));
        unary.propagate_type();
        Expr::UB ub = unary.propagate_value();
        std::shared_ptr<Expr> ret = std::make_shared<UnaryExpr> (unary);
        if (ub)
            ret = rebuild_unary(ub, ret);
        return ret;
    }
    else { // Binary expr
        BinaryExpr binary;
        std::uniform_int_distribution<int> op_type_dis(0, BinaryExpr::Op::MaxOp - 1);
        BinaryExpr::Op op_type = (BinaryExpr::Op) op_type_dis(rand_gen);
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
    return NULL;
}

std::shared_ptr<Expr> ArithExprGen::rebuild_unary (Expr::UB ub, std::shared_ptr<Expr> expr) {
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

std::shared_ptr<Expr> ArithExprGen::rebuild_binary (Expr::UB ub, std::shared_ptr<Expr> expr) {
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
//                std::cout << "max_sht_val " << max_sht_val << std::endl;
                std::uniform_int_distribution<uint64_t> const_val_dis(0, max_sht_val);
                uint64_t const_val = const_val_dis(rand_gen);
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
//                std::cout << "ret " << ret->emit() << std::endl;
//                std::cout << "ins " << ins.emit() << std::endl;
//                std::cout << "ins val " << ins.get_value() << std::endl;
//                std::cout << "ret lhs " << lhs->get_value() << std::endl;
//                std::cout << "ret rhs " << rhs->get_value() << std::endl;
//                std::cout << "ret lhs type " << lhs->get_type_id() << std::endl;
//                std::cout << "ret rhs type " << rhs->get_type_id() << std::endl;
                ret->set_rhs(std::make_shared<BinaryExpr> (ins));
//                std::cout << "ret new " << ret->emit() << std::endl;
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

std::shared_ptr<Stmnt> ArithExprGen::get_expr_stmnt () {
    ExprStmnt ret;
    ret.set_expr(res_expr);
    return std::make_shared<ExprStmnt> (ret);
}

void ArrayGen::generate () {
    std::uniform_int_distribution<int> type_dis(0, ctrl.allowed_types.size() - 1);
    std::uniform_int_distribution<int> size_dis(ctrl.min_arr_size, ctrl.max_arr_size);
    std::uniform_int_distribution<int> ess_dis(0, Array::Ess::MAX_ESS - 1);
    // TODO: add static and modifier option

    Type::TypeID type = ctrl.allowed_types.at(type_dis(rand_gen));
    int size = size_dis(rand_gen);
    Array::Ess ess = (Array::Ess) (ESSENCE_DIFFER ? ess_dis(rand_gen) : ctrl.primary_ess);

    Array inp = Array ("in_" + ctrl.ext_num, type, Variable::Mod::NTHNG, false, size, ess);
    inp.set_align (32);
    ControlStruct tmp_ctrl = ctrl;
    tmp_ctrl.min_var_val = inp.get_base_type()->get_min();
    tmp_ctrl.max_var_val = inp.get_base_type()->get_max();
    VarValGen var_val_gen (tmp_ctrl, inp.get_base_type()->get_id());
    var_val_gen.generate();
    inp.set_value(var_val_gen.get_value());
    inp_arr = std::make_shared<Array> (inp);
    sym_table.push_back(std::make_shared<Array> (inp));

    type = ctrl.allowed_types.at(type_dis(rand_gen));
    size = size_dis(rand_gen);
    ess = (Array::Ess) (ESSENCE_DIFFER ? ess_dis(rand_gen) : ctrl.primary_ess);

    Array out = Array ("out_" + ctrl.ext_num, type, Variable::Mod::NTHNG, false, size, ess);
    out.set_align (32);
    tmp_ctrl = ctrl;
    tmp_ctrl.min_var_val = out.get_base_type()->get_min();
    tmp_ctrl.max_var_val = out.get_base_type()->get_max();
    var_val_gen = VarValGen (tmp_ctrl, out.get_base_type()->get_id());
    var_val_gen.generate();
    out.set_value(var_val_gen.get_value());
    out_arr = std::make_shared<Array> (out);
    sym_table.push_back(std::make_shared<Array> (out));
}

void VarValGen::generate () {
    switch (type_id) {
        case Type::TypeID::BOOL:
            {
                std::uniform_int_distribution<int> B_dis((bool) ctrl.min_var_val, (bool) ctrl.max_var_val);
                val = (bool) B_dis (rand_gen);
            }
            break;
        case Type::TypeID::CHAR:
            {
                std::uniform_int_distribution<signed char> SC_dis((signed char) ctrl.min_var_val, (signed char) ctrl.max_var_val);
                val = (signed char) SC_dis (rand_gen);
            }
            break;
        case Type::TypeID::UCHAR:
            {
                std::uniform_int_distribution<unsigned char> UC_dis((unsigned char) ctrl.min_var_val, (unsigned char) ctrl.max_var_val);
                val = (unsigned char) UC_dis (rand_gen);
            }
            break;
        case Type::TypeID::SHRT:
            {
                std::uniform_int_distribution<short> S_dis((short) ctrl.min_var_val, (short) ctrl.max_var_val);
                val = (short) S_dis (rand_gen);
            }
            break;
        case Type::TypeID::USHRT:
            {
                std::uniform_int_distribution<unsigned short> US_dis((unsigned short) ctrl.min_var_val, (unsigned short) ctrl.max_var_val);
                val = (unsigned short) US_dis (rand_gen);
            }
            break;
        case Type::TypeID::INT:
            {
                std::uniform_int_distribution<int> I_dis((int) ctrl.min_var_val, (int) ctrl.max_var_val);
                val = (int) I_dis (rand_gen);
            }
            break;
        case Type::TypeID::UINT:
            {
                std::uniform_int_distribution<unsigned int> UI_dis((unsigned int) ctrl.min_var_val, (unsigned int) ctrl.max_var_val);
                val = (unsigned int) UI_dis (rand_gen);
            }
            break;
        case Type::TypeID::LINT:
            {
                std::uniform_int_distribution<long int> LI_dis((long int) ctrl.min_var_val, (long int) ctrl.max_var_val);
                val = (long int) LI_dis (rand_gen);
            }
            break;
        case Type::TypeID::ULINT:
            {
                std::uniform_int_distribution<unsigned long int> ULI_dis((unsigned long int) ctrl.min_var_val, (unsigned long int) ctrl.max_var_val);
                val = (unsigned long int) ULI_dis (rand_gen);
            }
            break;
        case Type::TypeID::LLINT:
            {
                std::uniform_int_distribution<long long int> LLI_dis((long long int) ctrl.min_var_val, (long long int) ctrl.max_var_val);
                val = (long long int) LLI_dis (rand_gen);
            }
            break;
        case Type::TypeID::ULLINT:
            {
                std::uniform_int_distribution<unsigned long long int> ULLI_dis((unsigned long long int) ctrl.min_var_val, (unsigned long long int) ctrl.max_var_val);
                val = (unsigned long long int) ULLI_dis (rand_gen);
            }
            break;
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VarValGen: generate" << std::endl;
            break;
    }
}

void VarValGen::generate_step () {
    generate();

    std::uniform_int_distribution<int> step_dis(0, 100);
    unsigned int step_dis_val = step_dis(rand_gen);
    int64_t tmp_val = 0;
    if (step_dis_val < 15) {
        tmp_val = 1;
    }
    else if (step_dis_val < 30) {
        tmp_val = 2;
    }
    else if (step_dis_val < 45) {
        tmp_val = 3;
    }
    else if (step_dis_val < 60) {
        tmp_val = 4;
    }
    else if (step_dis_val < 75) {
        tmp_val = 8;
    }
    else {
        return;
    }
    tmp_val = ((int64_t)ctrl.max_var_val < 0) ? -1 * tmp_val : tmp_val;// Negative step
    if ((int64_t)ctrl.min_var_val <= tmp_val && tmp_val <= (int64_t)ctrl.max_var_val)
        val = tmp_val;
}

void StepExprGen::generate () {
    VarUseExpr var_use;
    var_use.set_variable (var);

    std::uniform_int_distribution<int> inc_dis(0, 100);

    if ((step != 1 && step != -1) || inc_dis(rand_gen) < 33) {
        ConstExpr step_val_expr;
        step_val_expr.set_type(var->get_type()->get_id());
        step_val_expr.set_data(var->get_value());

        BinaryExpr add_expr;
        add_expr.set_op(BinaryExpr::Op::Add);
        add_expr.set_lhs(std::make_shared<VarUseExpr> (var_use));
        add_expr.set_rhs(std::make_shared<ConstExpr> (step_val_expr));

        AssignExpr assign;
        assign.set_to(std::make_shared<VarUseExpr> (var_use));
        assign.set_from(std::make_shared<BinaryExpr> (add_expr));

        expr = std::make_shared<AssignExpr> (assign);
    }
    else {
        UnaryExpr step_expr;
        std::uniform_int_distribution<int> pre_post_dis(0, 1);
        if (pre_post_dis(rand_gen)) { // Pre
            step_expr.set_op(step == 1 ? UnaryExpr::Op::PreInc : UnaryExpr::Op::PreDec);
        }
        else { // Post
            step_expr.set_op(step == 1 ? UnaryExpr::Op::PostInc : UnaryExpr::Op::PostDec);
        }
        step_expr.set_arg(std::make_shared<VarUseExpr> (var_use));
        expr = std::make_shared<UnaryExpr> (step_expr);
    }
}

void Master::write_file (std::string of_name, std::string data) {
    std::ofstream out_file;
    out_file.open (out_folder + "/" + of_name);
    out_file << data;
    out_file.close ();
}

std::string Master::emit_main () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";
    ret += "extern void init ();\n";
    ret += "extern void foo ();\n";
    ret += "extern unsigned long long int checksum ();\n";
    ret += "int main () {\n";
    ret += "    init ();\n";
    ret += "    foo ();\n";
    ret += "    std::cout << checksum () << std::endl;\n";
    ret += "    return 0;\n";
    ret += "}";
    write_file("driver.cpp", ret);
    return ret;
}

std::string Master::emit_func () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";
    ret += "void foo () {\n";
    for (auto i = program.begin(); i != program.end(); ++i) {
        ret += (*i)->emit() + "\n";
    }
    ret += "}";
    write_file("func.cpp", ret);
    return ret;
}

std::string Master::emit_loop (std::shared_ptr<Data> arr, std::shared_ptr<FuncCallExpr> func_call) {
    std::string ret = "";

    Variable iter = Variable("i", Type::TypeID::INT, Variable::Mod::NTHNG, false);
    iter.set_value (0);

    VarUseExpr iter_use;
    iter_use.set_variable(std::make_shared<Variable>(iter));

    IndexExpr arr_use;
    arr_use.set_base(std::static_pointer_cast<Array>(arr));
    arr_use.set_index(std::make_shared<VarUseExpr>(iter_use));

    ExprStmnt init_stmnt;
    if (func_call == NULL) {
        ConstExpr init_val;
        init_val.set_type(std::static_pointer_cast<Array>(arr)->get_base_type()->get_id());
        init_val.set_data((arr)->get_value());

        AssignExpr init_expr;
        init_expr.set_to(std::make_shared<IndexExpr>(arr_use));
        init_expr.set_from(std::make_shared<ConstExpr>(init_val));

        init_stmnt.set_expr(std::make_shared<AssignExpr>(init_expr));
    }
    else {
        func_call->add_to_args(std::make_shared<IndexExpr>(arr_use));
        init_stmnt.set_expr(func_call);
    }

    ConstExpr iter_init;
    iter_init.set_type(Type::TypeID::INT);
    iter_init.set_data(0);

    DeclStmnt iter_decl;
    iter_decl.set_data(std::make_shared<Variable>(iter));
    iter_decl.set_init(std::make_shared<ConstExpr>(iter_init));

    ConstExpr trip_val;
    trip_val.set_type(Type::TypeID::INT);
    trip_val.set_data(std::static_pointer_cast<Array>(arr)->get_size());

    BinaryExpr cond;
    cond.set_op(BinaryExpr::Op::Lt);
    cond.set_lhs (std::make_shared<VarUseExpr> (iter_use));
    cond.set_rhs (std::make_shared<ConstExpr> (trip_val));

    UnaryExpr step;
    step.set_op (UnaryExpr::Op::PreInc);
    step.set_arg (std::make_shared<VarUseExpr> (iter_use));

    CntLoopStmnt init_loop;
    init_loop.set_loop_type(LoopStmnt::LoopID::FOR);
    init_loop.add_to_body(std::make_shared<ExprStmnt> (init_stmnt));
    init_loop.set_cond(std::make_shared<BinaryExpr> (cond));
    init_loop.set_iter(std::make_shared<Variable> (iter));
    init_loop.set_iter_decl(std::make_shared<DeclStmnt> (iter_decl));
    init_loop.set_step_expr (std::make_shared<UnaryExpr> (step));

    return init_loop.emit();
}

std::string Master::emit_init () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";

    for (auto i = inp_sym_table.begin(); i != inp_sym_table.end(); ++i) {
        DeclStmnt decl;
        decl.set_data(*i);
        decl.set_is_extern(false);
        ret += decl.emit() + ";\n";
    }

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        DeclStmnt decl;
        decl.set_data(*i);
        decl.set_is_extern(false);
        ret += decl.emit() + ";\n";
    }

    ret += "void init () {\n";

    for (auto i = inp_sym_table.begin(); i != inp_sym_table.end(); ++i) {
        ret += emit_loop(*i) + "\n";
    }

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        ret += emit_loop(*i) + "\n";
    }

    ret += "}";
    write_file("init.cpp", ret);
    return ret;
}

std::string Master::emit_decl () {
    std::string ret = "";
    ret += "#include <cstdint>\n";
    ret += "#include <iostream>\n";
    ret += "#include <array>\n";
    ret += "#include <vector>\n";

    ret += "void hash(unsigned long long int &seed, unsigned long long int const &v);\n";

    for (auto i = inp_sym_table.begin(); i != inp_sym_table.end(); ++i) {
        DeclStmnt decl;
        decl.set_data(*i);
        decl.set_is_extern(true);
        ret += decl.emit() + ";\n";
    }

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        DeclStmnt decl;
        decl.set_data(*i);
        decl.set_is_extern(true);
        ret += decl.emit() + ";\n";
    }

    write_file("init.h", ret);
    return ret;
}

std::string Master::emit_hash () {
    std::string ret = "#include <functional>\n";
    ret += "void hash(unsigned long long int &seed, unsigned long long int const &v) {\n";
    ret += "    seed ^= v + 0x9e3779b9 + (seed<<6) + (seed>>2);\n";
    ret += "}\n";
    write_file("hash.cpp", ret);
    return ret;
}

std::string Master::emit_check () { // TODO: rewrite with IR
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";

    ret += "unsigned long long int checksum () {\n";

    Variable seed = Variable("seed", Type::TypeID::ULLINT, Variable::Mod::NTHNG, false);
    VarUseExpr seed_use;
    seed_use.set_variable (std::make_shared<Variable> (seed));

    ConstExpr zero_init;
    zero_init.set_type (Type::TypeID::ULLINT);
    zero_init.set_data (0);

    DeclStmnt seed_decl;
    seed_decl.set_data (std::make_shared<Variable> (seed));
    seed_decl.set_init (std::make_shared<ConstExpr> (zero_init));

    ret += seed_decl.emit() + ";\n";

    ExprListExpr hash_args;
    hash_args.add_to_list(std::make_shared<VarUseExpr> (seed_use));

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        FuncCallExpr hash_func;
        hash_func.set_name("hash");
        hash_func.set_args(std::make_shared<ExprListExpr> (hash_args));
        ret += emit_loop(*i, std::make_shared<FuncCallExpr>(hash_func)) + "\n";
    }
    ret += "    return seed;\n";
    ret += "}";
    write_file("check.cpp", ret);
    return ret;
}
