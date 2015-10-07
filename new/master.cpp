#include "master.h"

const int MIN_LOOP_NUM = 3;
const int MAX_LOOP_NUM = 10;

const int MIN_ARR_NUM = 1;
const int MAX_ARR_NUM = 10;

const int MIN_ARR_SIZE = 1000;
const int MAX_ARR_SIZE = 10000;

const int NUM_OF_MIXED_TYPES = 3;

const bool ESSENCE_DIFFER = false;

const int MAX_ARITH_DEPTH = 3;

uint64_t rand_dev () {
//    return 149370038; // TODO: enable random
    std::random_device rd;
    uint64_t ret = rd ();
    std::cout << "/*SEED " << ret << "*/\n";
    return ret;
}

std::mt19937_64 rand_gen(rand_dev());

Master::Master () {
    ctrl.ext_num = "";

    int gen_types = 0;
    std::uniform_int_distribution<int> type_dis(0, Type::TypeID::MAX_INT_ID - 1);
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
}

void Master::generate () {
    // choose num of loops
    std::uniform_int_distribution<int> loop_num_dis(MIN_LOOP_NUM, MAX_LOOP_NUM);
    int loop_num = loop_num_dis(rand_gen);
    for (int i = 0; i < loop_num; ++i) {
        ctrl.ext_num = "lp" + std::to_string(i);
        LoopGen loop_gen (ctrl);
        loop_gen.generate();
        sym_table.insert(sym_table.end(), loop_gen.get_sym_table().begin(), loop_gen.get_sym_table().end());
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

    for (auto i = out_sym_table.begin(); i != out_sym_table.end(); ++i) {
        IndexExpr out_arr_index;
        out_arr_index.set_index(std::make_shared<VarUseExpr> (iter_use));
        out_arr_index.set_base(std::static_pointer_cast<Array>(*i));

        ArithExprGen arith_expr_gen (ctrl, inp_expr, std::make_shared<IndexExpr> (out_arr_index));
        arith_expr_gen.generate();
        loop.add_to_body(arith_expr_gen.get_expr_stmnt());
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
    int64_t max_arr_indx = std::min(iter->get_max (), min_ex_arr_size) - 1; // TODO: I hope it will work
    tmp_ctrl.max_var_val= max_arr_indx;
    VarValGen var_val_gen (tmp_ctrl, var_type);
    var_val_gen.generate();
    int64_t start_val = var_val_gen.get_value();
    iter->set_min(start_val);

    // Step
    std::uniform_int_distribution<int> step_dir_dis(0, 1);
    int step_dir = step_dir_dis(rand_gen) ? 1 : -1;
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
    if (step_dir < 0) { // Negative step
        max_step_num = std::abs(start_val / step); // Trip from 0 to start_val
    }
    else { // Positive step
        max_step_num = std::abs((max_arr_indx - start_val) / step); // Trip from start_val to max_arr_indx
    }
    std::uniform_int_distribution<int> step_num_dis(1, max_step_num);
    int64_t step_num = step_num_dis(rand_gen);

    // End value
    std::uniform_int_distribution<int> end_diff_dis(0, 1);
    bool hit_end = end_diff_dis(rand_gen);
    int64_t end_value = start_val + step_num * step;
    end_value += hit_end ? 0 : step / 2;
    if (end_value < 0L) {
        end_value = std::max(end_value, 0L);
        hit_end = false;
    }
    if (end_value > max_arr_indx) {
        end_value = std::min(end_value, max_arr_indx);
        hit_end = false;
    }
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
        allowed_cmp_op.push_back(BinaryExpr::Op::Gt);
        allowed_cmp_op.push_back(BinaryExpr::Op::Ge);
    }
    else {
        allowed_cmp_op.push_back(BinaryExpr::Op::Lt);
        allowed_cmp_op.push_back(BinaryExpr::Op::Le);
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
        unary.propagate_value();
        return std::make_shared<UnaryExpr> (unary);
    }
    else { // Binary expr
        BinaryExpr binary;
        std::uniform_int_distribution<int> op_type_dis(0, BinaryExpr::Op::MaxOp - 1);
        BinaryExpr::Op op_type = (BinaryExpr::Op) op_type_dis(rand_gen);
        binary.set_op(op_type);
        binary.set_lhs(gen_level(depth + 1));
        binary.set_rhs(gen_level(depth + 1));
        binary.propagate_type();
        binary.propagate_value();
        return std::make_shared<BinaryExpr> (binary);
    }
    return NULL;
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
    tmp_ctrl.min_var_val = inp.get_type()->get_min();
    tmp_ctrl.max_var_val = inp.get_type()->get_max();
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
    tmp_ctrl.min_var_val = out.get_type()->get_min();
    tmp_ctrl.max_var_val = out.get_type()->get_max();
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

std::string Master::emit () {
    std::string ret = "";

    for (auto i = sym_table.begin(); i != sym_table.end(); ++i) {
        DeclStmnt decl;
        decl.set_data(*i);
        ret += decl.emit() + ";\n";
    }

    for (auto i = program.begin(); i != program.end(); ++i) {
        ret += (*i)->emit() + "\n";
    }
    return ret;
}
