#include "master.h"

const int MIN_LOOP_NUM = 3;
const int MAX_LOOP_NUM = 10;

const int MIN_ARR_NUM = 1;
const int MAX_ARR_NUM = 10;

const int MIN_ARR_SIZE = 1000;
const int MAX_ARR_SIZE = 10000;

const int NUM_OF_MIXED_TYPES = 3;

const bool ESSENCE_DIFFER = false;

uint64_t rand_dev () {
//    return -954396712; // TODO: enable random
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
}

void Master::generate () {
    // choose num of loops
    std::uniform_int_distribution<int> loop_num_dis(MIN_LOOP_NUM, MAX_LOOP_NUM);
    int loop_num = loop_num_dis(rand_gen);
    for (int i = 0; i < loop_num; ++i) {
        ctrl.ext_num = "lp" + std::to_string(i);
        LoopGen loop_gen (ctrl);
        loop_gen.generate();
        sym_table.insert(sym_table.end(), loop_gen.get_sym_table().begin(), loop_gen.get_sym_table().end() );
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
        min_arr_size = std::min(min_arr_size, inp_arr->get_size());
        inp_sym_table.push_back(inp_arr);
        sym_table.push_back(inp_arr);

        std::shared_ptr<Array> out_arr = arr_gen.get_out_arr();
        min_arr_size = std::min(min_arr_size, out_arr->get_size());
        out_sym_table.push_back(out_arr);
        sym_table.push_back(out_arr);
    }

    // LoopType
    CntLoopStmnt loop;
    std::uniform_int_distribution<int> loop_id_dis(0, LoopStmnt::LoopID::MAX_LOOP_ID - 1);
    LoopStmnt::LoopID loop_type = (LoopStmnt::LoopID) loop_id_dis(rand_gen);
    loop.set_loop_type (loop_type);

    // Iterator
    std::uniform_int_distribution<int> type_dis(0, ctrl.allowed_types.size() - 1);
    Type::TypeID var_type;
    Variable iter = Variable("i_" + ctrl.ext_num, type, Variable::Mod::NTHNG, false);
    // set min and max
    // move step value to iter value ?

    // Condition
    BinaryExpr cond;
    // set cond type
    std::uniform_int_distribution<int> cond_dis(BinaryExpr::Op::Lt, BinaryExpr::Ne); // TODO: we want to use variable as condition
    BinaryExpr::Op cond_type;
    cond.set_lhs (std::make_shared<VarUseExpr> (var_use));
    // set rhs as step

    bool incomp_data;
    do {
        incomp_data = false;

        var_type = ctrl.allowed_types.at(type_dis(rand_gen));
        iter = Variable("i_" + ctrl.ext_num, type, Variable::Mod::NTHNG, false);

        cond_type = (BinaryExpr::Op) cond_dis(rand_gen);

        uint64_t start_val = 
    } while (incomp_data);
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
    inp_arr = std::make_shared<Array> (inp);
    sym_table.push_back(std::make_shared<Array> (inp));

    type = ctrl.allowed_types.at(type_dis(rand_gen));
    size = size_dis(rand_gen);
    ess = (Array::Ess) (ESSENCE_DIFFER ? ess_dis(rand_gen) : ctrl.primary_ess);

    Array out = Array ("out_" + ctrl.ext_num, type, Variable::Mod::NTHNG, false, size, ess);
    out.set_align (32);
    out_arr = std::make_shared<Array> (out);
    sym_table.push_back(std::make_shared<Array> (out));
}

std::string Master::emit () {
    std::string ret = "";
    for (auto i = sym_table.begin(); i != sym_table.end(); ++i) {
        DeclStmnt decl;
        decl.set_data(*i);
        ret += decl.emit() + ";\n";
    }
    return ret;
}
