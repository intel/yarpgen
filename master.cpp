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

const int MIN_ARR_NUM = 7;
const int MAX_ARR_NUM = 10;

const int MIN_ARR_SIZE = 1000;
const int MAX_ARR_SIZE = 10000;

const int NUM_OF_MIXED_TYPES = 3;

const bool ESSENCE_DIFFER = false;

const int MAX_ARITH_DEPTH = 3;

const int MAX_TMP_VAR_NUM = 3;
const int MAX_TMP_ARITH_DEPTH = 2;

std::shared_ptr<RandValGen> rand_val_gen;

ReductionPolicy::ReductionPolicy () {
    reduction_mode = false;
    phase = INT_MIN;
    reduce_loop = false;
    reduce_result = false;
    prev_loop = INT_MIN;

    reduce_body = false;
    prev_stmt = INT_MIN;
}

void ReductionPolicy::init (bool _reduction_mode, std::string out_folder) {
    if (!_reduction_mode)
        return;
    reduction_mode = _reduction_mode;
    reduce_loop = reduction_mode;

    struct stat buffer;
    std::string reduce_log_file = out_folder + "/" + "reduce_log.txt";
    if (stat (reduce_log_file.c_str(), &buffer) != 0) { // reduce_log.txt doesn't exist
        phase = 1;
        prev_loop = INT_MAX;
        reduce_result = true;
        return;
    }

    // Read loop reduction parametres
    std::ifstream file(reduce_log_file.c_str());
    std::string str;

    std::getline(file, str);
    std::getline(file, str);
    phase = std::stoi(str);

    std::getline(file, str);
    std::getline(file, str);
    std::istringstream omit_iss (str);
    int n;
    while (omit_iss >> n) {
        omitted_loop.push_back(n);
    }

    std::getline(file, str);
    std::getline(file, str);
    std::istringstream susp_iss (str);
    while (susp_iss >> n) {
        suspect_loop.push_back(n);
    }

    std::getline(file, str);
    std::getline(file, str);
    prev_loop = std::stoi (str);

    if (phase == 1 && prev_loop == -1) {
        reduce_loop = false;

        phase = 2;
        reduce_body = true;
        prev_loop = suspect_loop.at(suspect_loop.size() - 1);
        prev_stmt = INT_MAX;

        reduce_result = true;
        return;
    }

    if (phase == 2) {
        reduce_loop = false;
        reduce_body = true;

        std::getline(file, str);
        std::getline(file, str);
        std::istringstream omit_stmt_iss (str);
        while (omit_stmt_iss >> n) {
            omitted_stmt.push_back(n);
        }

        std::getline(file, str);
        std::getline(file, str);
        std::istringstream susp_stmt_iss (str);
        while (susp_stmt_iss >> n) {
            suspect_stmt.push_back(n);
        }

        std::getline(file, str);
        std::getline(file, str);
        prev_stmt = std::stoi (str);
    }

    std::getline(file, str);
    std::getline(file, str);
    reduce_result = std::stoi (str);

    if (phase == 1) {
      if (reduce_loop && reduce_result)
         omitted_loop.push_back(prev_loop);
      else
         suspect_loop.push_back(prev_loop);
    }

    if (phase == 2) {
      if (reduce_body && reduce_result)
         omitted_stmt.push_back(prev_stmt);
      else
         suspect_stmt.push_back(prev_stmt);
    }
}

ReductionPolicy red_policy;

RandValGen::RandValGen (uint64_t _seed) {
    if (_seed != 0) {
        seed = _seed;
    }
    else {
        std::random_device rd;
        seed = rd ();
    }
    std::cout << "/*SEED " << seed << "*/\n";
    rand_gen = std::mt19937_64(seed);
}

template<typename T>
T RandValGen::get_rand_value (T from, T to) {
    std::uniform_int_distribution<T> dis(from, to);
    return dis(rand_gen);
}

Master::Master (std::string _out_folder, bool reduce_mode) {
    out_folder = _out_folder;

    gen_policy.ext_num = "";

    int gen_types = 0;
    while (gen_types < NUM_OF_MIXED_TYPES) {
        Type::TypeID type = (Type::TypeID) rand_val_gen->get_rand_value<int>(Type::TypeID::CHAR, Type::TypeID::MAX_INT_ID - 1);
        if (std::find(gen_policy.allowed_types.begin(), gen_policy.allowed_types.end(), type) == gen_policy.allowed_types.end()) {
            gen_policy.allowed_types.push_back (type);
            gen_types++;
        }
    }

    gen_policy.min_arr_num = MIN_ARR_NUM;
    gen_policy.max_arr_num = MAX_ARR_NUM;
    gen_policy.min_arr_size = MIN_ARR_SIZE;
    gen_policy.max_arr_size = MAX_ARR_SIZE;

    gen_policy.primary_ess = (Array::Ess) rand_val_gen->get_rand_value<int>(0, Array::Ess::MAX_ESS - 1);

    gen_policy.min_var_val = 0;
    gen_policy.max_var_val = UINT_MAX;

    gen_policy.max_arith_depth = MAX_ARITH_DEPTH;

    gen_policy.max_tmp_var_num = MAX_TMP_VAR_NUM;
    gen_policy.max_tmp_arith_depth = MAX_TMP_ARITH_DEPTH;

    gen_policy.self_dep = true;
    gen_policy.inter_war_dep = true;

    gen_policy.else_branch = true;

    red_policy.init(reduce_mode, out_folder);
}

void Master::generate () {
    // choose num of loops
    int loop_num = rand_val_gen->get_rand_value<int>(MIN_LOOP_NUM, MAX_LOOP_NUM);

    if (red_policy.reduction_mode && red_policy.reduce_loop) {
        if (red_policy.prev_loop == INT_MAX)
            red_policy.prev_loop = loop_num - 1;
        else
            red_policy.prev_loop = red_policy.prev_loop - 1;
    }

    for (int i = 0; i < loop_num; ++i) {
        gen_policy.ext_num = "lp" + std::to_string(i);
        LoopGen loop_gen (gen_policy);
        loop_gen.generate();
        if (red_policy.reduction_mode && 
           (std::find(red_policy.omitted_loop.begin(), red_policy.omitted_loop.end(), i) != red_policy.omitted_loop.end() || // We can delete loop
           (red_policy.reduce_loop && red_policy.prev_loop == i))) // We will try to delete loop
            continue;
        inp_sym_table.insert(inp_sym_table.end(), loop_gen.get_inp_sym_table().begin(), loop_gen.get_inp_sym_table().end());
        out_sym_table.insert(out_sym_table.end(), loop_gen.get_out_sym_table().begin(), loop_gen.get_out_sym_table().end());
        program.insert(program.end(), loop_gen.get_program().begin(), loop_gen.get_program().end());
    }
}

void LoopGen::generate () {
    // Array
    int arr_num = rand_val_gen->get_rand_value<int>(gen_policy.min_arr_num, gen_policy.max_arr_num);;
    for (int i = 0; i < arr_num; ++i) {
        GenerationPolicy tmp_gen_policy = gen_policy;
        tmp_gen_policy.ext_num += "_" + std::to_string(i);
        ArrayGen arr_gen (tmp_gen_policy);
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
    CntLoopStmt loop;
    LoopStmt::LoopID loop_type = (LoopStmt::LoopID) rand_val_gen->get_rand_value<int>(0, LoopStmt::LoopID::MAX_LOOP_ID - 1);
    loop.set_loop_type (loop_type);

///////////////////////////////////////////////////////////////////////////////////////////////////
    // Trip
    TripGen trip_gen (gen_policy, min_ex_arr_size);
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

    std::vector<BodyRet> body_ret = body_gen(inp_expr, out_expr, true);

    bool actual_loop = gen_policy.ext_num == ("lp" + std::to_string(red_policy.prev_loop));
    if (red_policy.reduce_body && actual_loop) {
        if (red_policy.prev_stmt == INT_MAX)
            red_policy.prev_stmt = body_ret.size() - 1;
        else
            red_policy.prev_stmt = red_policy.prev_stmt - 1;
    }

    for (int i = 0; i < body_ret.size(); ++i) {
        if (red_policy.reduction_mode && 
           (std::find(red_policy.omitted_stmt.begin(), red_policy.omitted_stmt.end(), i) != red_policy.omitted_stmt.end() || // We can delete loop
           (red_policy.reduce_body && red_policy.prev_stmt == i))) // We will try to delete loop
            continue;
        loop.add_to_body(body_ret.at(i).stmt);
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
    program.push_back(std::make_shared<CntLoopStmt> (loop));
}

std::vector<LoopGen::BodyRet> LoopGen::body_gen (std::vector<std::shared_ptr<Expr>> inp_expr,
                                        std::vector<std::shared_ptr<Expr>> out_expr,
                                        bool form_avail_inp) {
    std::vector<BodyRet> ret;

    std::vector<std::shared_ptr<Expr>> avail_inp_expr;
    if (form_avail_inp && gen_policy.inter_war_dep) {
        avail_inp_expr = out_expr;
        avail_inp_expr.insert(avail_inp_expr.end(), inp_expr.begin(), inp_expr.end());
    }
    else { // Self-dependency and no dependency
        avail_inp_expr = inp_expr;
    }

    bool if_exist = rand_val_gen->get_rand_value<int>(0, 100) > 50 ? true : false;
    int if_start = rand_val_gen->get_rand_value<int>(1, out_expr.size() - if_start);
    int if_end = rand_val_gen->get_rand_value<int>(1, out_expr.size() - if_start) + if_start;

    IfStmt if_stmt;
//    std::cerr << "if_exist " << if_exist << std::endl;
//    std::cerr << "if_start " << if_start << std::endl;
//    std::cerr << "if_end " << if_end << std::endl;
    if_stmt.set_else_exist(gen_policy.else_branch && rand_val_gen->get_rand_value<int>(0, 1));

    for (int i = 0; i < out_expr.size(); ++i) {
//        std::cerr << "i start " << i << std::endl;
        if (gen_policy.inter_war_dep && !gen_policy.self_dep)
            avail_inp_expr.erase(avail_inp_expr.begin());
        else if (!gen_policy.inter_war_dep && gen_policy.self_dep)
            avail_inp_expr.insert(avail_inp_expr.begin(), out_expr.at(i));

        int create_tmp_var = rand_val_gen->get_rand_value<int>(0, 100) > 70 ? true : false;
        create_tmp_var = create_tmp_var && (tmp_var_num < gen_policy.max_tmp_var_num);
        if (create_tmp_var) {
            GenerationPolicy tmp_gen_policy = gen_policy;
            tmp_gen_policy.max_arith_depth = gen_policy.max_tmp_arith_depth;

            TmpVariableGen tmp_variable_gen (tmp_gen_policy, avail_inp_expr, tmp_var_num);
            tmp_variable_gen.generate();
            BodyRet var_decl;
            var_decl.stmt = tmp_variable_gen.get_var_decl();
            var_decl.undel = 1;
            ret.push_back(var_decl);
            avail_inp_expr.push_back(tmp_variable_gen.get_var_use());
            tmp_var_num++;
        }

        if (if_exist && if_start == i) {
            ArithExprGen arith_expr_gen (gen_policy, avail_inp_expr, NULL);
            arith_expr_gen.generate();
            if_stmt.set_cond(arith_expr_gen.get_expr());
            std::vector<std::shared_ptr<Expr>>::const_iterator first = out_expr.begin() + if_start;
            std::vector<std::shared_ptr<Expr>>::const_iterator last = out_expr.begin() + if_end;
            std::vector<std::shared_ptr<Expr>> if_out_expr(first, last);
//            std::cerr << "if_out_expr.size() " << if_out_expr.size() << std::endl;
            std::vector<BodyRet> if_body_ret = body_gen(avail_inp_expr, if_out_expr, false);
//            std::cerr << "if_body.size() " << if_body.size() << std::endl;
            if (!if_stmt.get_else_exist()) {
                for (int j = 0; j < if_body_ret.size(); ++j) {
                    if_stmt.add_if_stmt(if_body_ret.at(j).stmt);
                }
            }
            else {
                int add_to_branch [if_body_ret.size()];
                for (int j = 0; j < if_body_ret.size(); ++j) {
                    add_to_branch [j] = rand_val_gen->get_rand_value<int>(0, 100);
                }
                for (int j = 0; j < if_body_ret.size(); ++j) {
                    if (add_to_branch [j] <= 60 || if_body_ret.at(j).undel)
                        if_stmt.add_if_stmt(if_body_ret.at(j).stmt);
                }
                std::vector<BodyRet> else_body_ret = body_gen(avail_inp_expr, if_out_expr, false);
                for (int j = 0; j < else_body_ret.size(); ++j) {
                    if (add_to_branch [j] <= 30 || (60 <= add_to_branch [j] && add_to_branch [j] <= 90) || else_body_ret.at(j).undel)
                        if_stmt.add_else_stmt(else_body_ret.at(j).stmt);
                }
            }
            BodyRet if_ret;
            if_ret.stmt = std::make_shared<IfStmt>(if_stmt);
            ret.push_back(if_ret);
            i += if_end - if_start - 1;

            if (gen_policy.inter_war_dep && gen_policy.self_dep) {
                for (int j = if_start; j < if_end; ++j) {
                    avail_inp_expr.erase(avail_inp_expr.begin());
                }
            }
        }
        else {
            ArithExprGen arith_expr_gen (gen_policy, avail_inp_expr, out_expr.at(i));
            arith_expr_gen.generate();
            BodyRet arith_ret;
            arith_ret.stmt = arith_expr_gen.get_expr_stmt();
            ret.push_back(arith_ret);
            if (gen_policy.inter_war_dep && gen_policy.self_dep) {
                avail_inp_expr.erase(avail_inp_expr.begin());
            }
        }
//        std::cerr << "i = " << i << std::endl;
    }
    return ret;
}

void TripGen::generate () {
    Type::TypeID var_type = (Type::TypeID) rand_val_gen->get_rand_value<int>(Type::TypeID::CHAR, Type::TypeID::MAX_INT_ID - 1);
    Variable iter_var ("i_" + gen_policy.ext_num, var_type, Variable::Mod::NTHNG, false);
    iter = std::make_shared<Variable> (iter_var);

    // Start value
    GenerationPolicy tmp_gen_policy = gen_policy;
    tmp_gen_policy.min_var_val = 0;
    int64_t max_arr_indx = std::min(iter->get_max (), min_ex_arr_size - 1); // TODO: I hope it will work
    tmp_gen_policy.max_var_val= max_arr_indx;
    VarValGen var_val_gen (tmp_gen_policy, var_type);
    var_val_gen.generate();
    int64_t start_val = var_val_gen.get_value();
    iter->set_min(start_val);

    // Step
    int step_dir = rand_val_gen->get_rand_value<int>(0, 1) ? 1 : -1;
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
        tmp_gen_policy.min_var_val = -1 * start_val;
        tmp_gen_policy.max_var_val = -1;
    }
    if (step_dir > 0 || start_val == 0) { // Positive step
        tmp_gen_policy.min_var_val = 1;
        tmp_gen_policy.max_var_val = max_arr_indx - start_val;
    }
    var_val_gen = VarValGen (tmp_gen_policy, Type::TypeID::LLINT);
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
    int64_t step_num = rand_val_gen->get_rand_value<int>(1, max_step_num);

    // End value
    int64_t end_value = start_val + step_num * step;
    int64_t next_end_value = start_val + (step_num + 1) * step;
    bool can_iter_of = (next_end_value >= max_arr_indx) || (next_end_value < 0);
    bool hit_end = rand_val_gen->get_rand_value<int>(0, 1) || can_iter_of;
    end_value += hit_end ? 0 : step / 2;
    iter->set_max(end_value);

    StepExprGen step_expr_gen (gen_policy, iter, step);
    step_expr_gen.generate();
    step_expr = step_expr_gen.get_expr();

    gen_condition(hit_end, step);
}

std::shared_ptr<DeclStmt> TripGen::get_iter_decl () {
    ConstExpr iter_init;
    iter_init.set_type(iter->get_type()->get_id());
    iter_init.set_data(iter->get_min());
    DeclStmt iter_decl;
    iter_decl.set_data(iter);
    iter_decl.set_init(std::make_shared<ConstExpr> (iter_init));
    return std::make_shared<DeclStmt> (iter_decl);
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
    BinaryExpr::Op cmp_op = allowed_cmp_op.at(rand_val_gen->get_rand_value<int>(0, allowed_cmp_op.size() - 1));

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
    int node_type = rand_val_gen->get_rand_value<int>(0, 100);
    if (node_type < 10 || depth == gen_policy.max_arith_depth) { // Array
        int inp_use = rand_val_gen->get_rand_value<int>(0, inp.size() - 1);;
        inp.at(inp_use)->propagate_type();
        inp.at(inp_use)->propagate_value();
        return inp.at(inp_use);
    }
    else if (node_type < 40) { // Unary expr
        UnaryExpr unary;
        UnaryExpr::Op op_type = (UnaryExpr::Op) rand_val_gen->get_rand_value<int>(UnaryExpr::Op::Plus, UnaryExpr::Op::MaxOp - 1);
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
        BinaryExpr::Op op_type = (BinaryExpr::Op) rand_val_gen->get_rand_value<int>(0, BinaryExpr::Op::MaxOp - 1);
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

std::shared_ptr<Stmt> ArithExprGen::get_expr_stmt () {
    ExprStmt ret;
    ret.set_expr(res_expr);
    return std::make_shared<ExprStmt> (ret);
}

void ArrayGen::generate () {
    // TODO: add static and modifier option
    Type::TypeID type = gen_policy.allowed_types.at(rand_val_gen->get_rand_value<int>(0, gen_policy.allowed_types.size() - 1));
    int size = rand_val_gen->get_rand_value<int>(gen_policy.min_arr_size, gen_policy.max_arr_size);
    Array::Ess ess = (Array::Ess) (ESSENCE_DIFFER ? rand_val_gen->get_rand_value<int>(0, Array::Ess::MAX_ESS - 1) : 
                                                    gen_policy.primary_ess);

    Array inp = Array ("in_" + gen_policy.ext_num, type, Variable::Mod::NTHNG, false, size, ess);
    inp.set_align (32);
    GenerationPolicy tmp_gen_policy = gen_policy;
    tmp_gen_policy.min_var_val = inp.get_base_type()->get_min();
    tmp_gen_policy.max_var_val = inp.get_base_type()->get_max();
    VarValGen var_val_gen (tmp_gen_policy, inp.get_base_type()->get_id());
    var_val_gen.generate();
    inp.set_value(var_val_gen.get_value());
    inp_arr = std::make_shared<Array> (inp);
    sym_table.push_back(std::make_shared<Array> (inp));

    type = gen_policy.allowed_types.at(rand_val_gen->get_rand_value<int>(0, gen_policy.allowed_types.size() - 1));
    size =  rand_val_gen->get_rand_value<int>(gen_policy.min_arr_size, gen_policy.max_arr_size);
    ess = (Array::Ess) (ESSENCE_DIFFER ? rand_val_gen->get_rand_value<int>(0, Array::Ess::MAX_ESS - 1) : 
                                         gen_policy.primary_ess);

    Array out = Array ("out_" + gen_policy.ext_num, type, Variable::Mod::NTHNG, false, size, ess);
    out.set_align (32);
    tmp_gen_policy = gen_policy;
    tmp_gen_policy.min_var_val = out.get_base_type()->get_min();
    tmp_gen_policy.max_var_val = out.get_base_type()->get_max();
    var_val_gen = VarValGen (tmp_gen_policy, out.get_base_type()->get_id());
    var_val_gen.generate();
    out.set_value(var_val_gen.get_value());
    out_arr = std::make_shared<Array> (out);
    sym_table.push_back(std::make_shared<Array> (out));
}

void VarValGen::generate () {
    switch (type_id) {
        case Type::TypeID::BOOL:
            {
                val = (bool) rand_val_gen->get_rand_value<int>((bool) gen_policy.min_var_val, (bool) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::CHAR:
            {
                val = (signed char) rand_val_gen->get_rand_value<signed char>((signed char) gen_policy.min_var_val, 
                                                                              (signed char) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::UCHAR:
            {
                val = (unsigned char) rand_val_gen->get_rand_value<unsigned char>((unsigned char) gen_policy.min_var_val,
                                                                                  (unsigned char) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::SHRT:
            {
                val = (short) rand_val_gen->get_rand_value<short>((short) gen_policy.min_var_val,
                                                                  (short) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::USHRT:
            {
                val = (unsigned short) rand_val_gen->get_rand_value<unsigned short>((unsigned short) gen_policy.min_var_val,
                                                                                    (unsigned short) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::INT:
            {
                val = (int) rand_val_gen->get_rand_value<int>((int) gen_policy.min_var_val,
                                                              (int) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::UINT:
            {
                val = (unsigned int) rand_val_gen->get_rand_value<unsigned int>((unsigned int) gen_policy.min_var_val,
                                                                                (unsigned int) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::LINT:
            {
                val = (long int) rand_val_gen->get_rand_value<long int>((long int) gen_policy.min_var_val,
                                                                        (long int) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::ULINT:
            {
                val = (unsigned long int) rand_val_gen->get_rand_value<unsigned long int>((unsigned long int) gen_policy.min_var_val,
                                                                                          (unsigned long int) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::LLINT:
            {
                val = (long long int) rand_val_gen->get_rand_value<long long int>((long long int) gen_policy.min_var_val,
                                                                                  (long long int) gen_policy.max_var_val);
            }
            break;
        case Type::TypeID::ULLINT:
            {
                val = (unsigned long long int) rand_val_gen->get_rand_value<unsigned long long int>((unsigned long long int) gen_policy.min_var_val,
                                                                                                    (unsigned long long int) gen_policy.max_var_val);
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

    unsigned int step_dis_val = rand_val_gen->get_rand_value<int>(0, 100);
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
    tmp_val = ((int64_t)gen_policy.max_var_val < 0) ? -1 * tmp_val : tmp_val;// Negative step
    if ((int64_t)gen_policy.min_var_val <= tmp_val && tmp_val <= (int64_t)gen_policy.max_var_val)
        val = tmp_val;
}

void StepExprGen::generate () {
    VarUseExpr var_use;
    var_use.set_variable (var);

    if ((step != 1 && step != -1) || rand_val_gen->get_rand_value<int>(0, 100) < 33) {
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
        if (rand_val_gen->get_rand_value<int>(0, 1)) { // Pre
            step_expr.set_op(step == 1 ? UnaryExpr::Op::PreInc : UnaryExpr::Op::PreDec);
        }
        else { // Post
            step_expr.set_op(step == 1 ? UnaryExpr::Op::PostInc : UnaryExpr::Op::PostDec);
        }
        step_expr.set_arg(std::make_shared<VarUseExpr> (var_use));
        expr = std::make_shared<UnaryExpr> (step_expr);
    }
}

void TmpVariableGen::generate () {
    Type::TypeID type = gen_policy.allowed_types.at(rand_val_gen->get_rand_value<int>(0, gen_policy.allowed_types.size() - 1));
    Variable tmp_var = Variable("tmp_var_" + gen_policy.ext_num + "_" + std::to_string(tmp_var_num), type, Variable::Mod::NTHNG, false);
    var_ptr = std::make_shared<Variable> (tmp_var);

    ArithExprGen arith_expr_gen (gen_policy, inp_expr, NULL);
    arith_expr_gen.generate();
    init_ptr = arith_expr_gen.get_expr();
    var_ptr->set_value(init_ptr->get_value());

    VarUseExpr tmp_var_use;
    tmp_var_use.set_variable(var_ptr);
    var_use_ptr = std::make_shared<VarUseExpr> (tmp_var_use);

    DeclStmt tmp_var_decl;
    tmp_var_decl.set_data(var_ptr);
    tmp_var_decl.set_init(init_ptr);
    var_decl_ptr = std::make_shared<DeclStmt> (tmp_var_decl);
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

    ExprStmt init_stmt;
    if (func_call == NULL) {
        ConstExpr init_val;
        init_val.set_type(std::static_pointer_cast<Array>(arr)->get_base_type()->get_id());
        init_val.set_data((arr)->get_value());

        AssignExpr init_expr;
        init_expr.set_to(std::make_shared<IndexExpr>(arr_use));
        init_expr.set_from(std::make_shared<ConstExpr>(init_val));

        init_stmt.set_expr(std::make_shared<AssignExpr>(init_expr));
    }
    else {
        func_call->add_to_args(std::make_shared<IndexExpr>(arr_use));
        init_stmt.set_expr(func_call);
    }

    ConstExpr iter_init;
    iter_init.set_type(Type::TypeID::INT);
    iter_init.set_data(0);

    DeclStmt iter_decl;
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

    CntLoopStmt init_loop;
    init_loop.set_loop_type(LoopStmt::LoopID::FOR);
    init_loop.add_to_body(std::make_shared<ExprStmt> (init_stmt));
    init_loop.set_cond(std::make_shared<BinaryExpr> (cond));
    init_loop.set_iter(std::make_shared<Variable> (iter));
    init_loop.set_iter_decl(std::make_shared<DeclStmt> (iter_decl));
    init_loop.set_step_expr (std::make_shared<UnaryExpr> (step));

    return init_loop.emit();
}

std::string Master::emit_init () {
    std::string ret = "";
    ret += "#include \"init.h\"\n\n";

    for (auto i = inp_sym_table.begin(); i != inp_sym_table.end(); ++i) {
        DeclStmt decl;
        decl.set_data(*i);
        decl.set_is_extern(false);
        ret += decl.emit() + ";\n";
    }

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        DeclStmt decl;
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
    ret += "#include <valarray>\n";

    ret += "void hash(unsigned long long int &seed, unsigned long long int const &v);\n";

    for (auto i = inp_sym_table.begin(); i != inp_sym_table.end(); ++i) {
        DeclStmt decl;
        decl.set_data(*i);
        decl.set_is_extern(true);
        ret += decl.emit() + ";\n";
    }

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        DeclStmt decl;
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

    DeclStmt seed_decl;
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

std::string Master::emit_reduce_log () {
    if (!red_policy.reduction_mode)
      return "";

    std::string ret = "";
    ret += "Phase:\n";
    ret += std::to_string(red_policy.phase);
    ret += "\nOmitted loops:\n";
    for (auto i = red_policy.omitted_loop.begin(); i != red_policy.omitted_loop.end(); ++i)
        ret += std::to_string(*(i)) + " ";
    ret += "\nSuspect loops:\n";
    for (auto i = red_policy.suspect_loop.begin(); i != red_policy.suspect_loop.end(); ++i)
        ret += std::to_string(*(i)) + " ";
    ret += "\nPrev loop:\n";
    ret += std::to_string(red_policy.prev_loop);

    if (red_policy.phase == 2) {
        ret += "\nOmitted stmts:\n";
        for (auto i = red_policy.omitted_stmt.begin(); i != red_policy.omitted_stmt.end(); ++i)
            ret += std::to_string(*(i)) + " ";
        ret += "\nSuspect stmts:\n";
        for (auto i = red_policy.suspect_stmt.begin(); i != red_policy.suspect_stmt.end(); ++i)
            ret += std::to_string(*(i)) + " ";
        ret += "\nPrev stmt:\n";
        ret += std::to_string(red_policy.prev_stmt);
        if (red_policy.prev_stmt == -1)
            ret = "DONE!\n";
    }

    write_file("reduce_log.txt", ret);
    return ret;
}
