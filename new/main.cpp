#include "type.h"
#include "variable.h"
#include "node.h"
#include "master.h"

int main (int argv, char* argc[]) {
/*
    std::shared_ptr<Type> type;
    type = Type::init (Type::TypeID::UINT);
    type->dbg_dump();

    Variable var = Variable("i", Type::TypeID::UINT, Variable::Mod::NTHNG, false);
    var.set_modifier (Variable::Mod::CONST);
    var.set_value (10);
    var.set_min (5);
    var.set_max (50);
    var.dbg_dump ();

    Array arr = Array ("a", Type::TypeID::UINT, Variable::Mod::CONST_VOLAT, true, 20,
                           Array::Ess::STD_VEC);
    arr.set_align(32);
    arr.dbg_dump();

    VarUseExpr var_use;
    var_use.set_variable (std::make_shared<Variable> (var));
    std::cout << "VarUseExpr: " << var_use.emit () << std::endl;

    AssignExpr assign;
    assign.set_to (std::make_shared<VarUseExpr> (var_use));
    assign.set_from  (std::make_shared<VarUseExpr> (var_use));
    std::cout << "AssignExpr: " << assign.emit () << std::endl;

    IndexExpr index;
    index.set_base (std::make_shared<Array> (arr));
    index.set_index (std::make_shared<VarUseExpr> (var_use));
    index.set_is_subscr(true);
    std::cout << "IndexExpr: " << index.emit () << std::endl;
    index.set_is_subscr(false);
    std::cout << "IndexExpr: " << index.emit () << std::endl;

    BinaryExpr bin_add;
    bin_add.set_op (BinaryExpr::Op::Add);
    bin_add.set_lhs (std::make_shared<VarUseExpr> (var_use));
    bin_add.set_rhs (std::make_shared<VarUseExpr> (var_use));
    std::cout << "BinaryExpr: " << bin_add.emit () << std::endl;

    AssignExpr assign2;
    assign2.set_to (std::make_shared<IndexExpr> (index));
    assign2.set_from  (std::make_shared<BinaryExpr> (bin_add));
    std::cout << "AssignExpr: " << assign2.emit () << std::endl;

    UnaryExpr unary;
    unary.set_op (UnaryExpr::Op::BitNot);
    unary.set_arg (std::make_shared<IndexExpr> (index));
    std::cout << "UnaryExpr: " << unary.emit () << std::endl;

    ConstExpr cnst;
    cnst.set_type (Type::TypeID::ULLINT);
    cnst.set_data (123321);
    std::cout << "ConstExpr: " << cnst.emit () << std::endl;

    DeclStmnt decl;
    decl.set_data (std::make_shared<Array> (arr));
    std::cout << "DeclStmnt: " << decl.emit () << std::endl;
    decl.set_is_extern (true);
    std::cout << "DeclStmnt: " << decl.emit () << std::endl;

    ExprStmnt ex_st;
    ex_st.set_expr (std::make_shared<AssignExpr>(assign2));
    std::cout << "ExprStmnt: " << ex_st.emit () << std::endl;

    BinaryExpr cond;
    cond.set_op(BinaryExpr::Op::Le);
    cond.set_lhs (std::make_shared<VarUseExpr> (var_use));
    cond.set_rhs (std::make_shared<ConstExpr> (cnst));

    UnaryExpr step;
    step.set_op (UnaryExpr::Op::PreInc);
    step.set_arg (std::make_shared<VarUseExpr> (var_use));

    DeclStmnt iter_decl;
    iter_decl.set_data (std::make_shared<Variable> (var));

    CntLoopStmnt cnt_loop;
    cnt_loop.set_loop_type(LoopStmnt::LoopID::FOR);
    cnt_loop.add_to_body(std::make_shared<ExprStmnt> (ex_st));
    cnt_loop.set_cond(std::make_shared<BinaryExpr> (cond));
    cnt_loop.set_iter(std::make_shared<Variable> (var));
    cnt_loop.set_iter_decl(std::make_shared<DeclStmnt> (iter_decl));
    cnt_loop.set_step_expr (std::make_shared<UnaryExpr> (step));
    std::cout << "CntLoopStmnt: " << cnt_loop.emit () << std::endl;

    TypeCastExpr tc;
    tc.set_type(Type::init(Type::TypeID::ULLINT));
    tc.set_expr(std::make_shared<UnaryExpr> (unary));
    std::cout << "TypeCastExpr: " << tc.emit () << std::endl;

    ExprListExpr expr_list;
    expr_list.add_to_list(std::make_shared<TypeCastExpr>(tc));
    expr_list.add_to_list(std::make_shared<UnaryExpr>(step));
    std::cout << "ExprListExpr: " << expr_list.emit () << std::endl;

    FuncCallExpr fc;
    fc.set_name("hash");
    fc.set_args(std::make_shared<ExprListExpr> (expr_list));
    std::cout << "FuncCallExpr: " << fc.emit () << std::endl;
*/
    if (argv < 2) {
        std::cerr << "Specify output folder!" << std::endl;
        return -1;
    }

    Master mas (argc [1]);
    mas.generate ();
    mas.emit_func ();
    mas.emit_init ();
    mas.emit_decl ();
    mas.emit_hash ();
    mas.emit_check ();
    mas.emit_main ();

/*
    ConstExpr lhs;
    lhs.set_type (Type::TypeID::LLINT);
    lhs.set_data (INT_MAX);
    lhs.propagate_type();
    lhs.propagate_value();
    std::cout << "ConstExpr: " << lhs.emit () << std::endl;

    ConstExpr rhs;
    rhs.set_type (Type::TypeID::LLINT);
    rhs.set_data(2);
    rhs.propagate_type();
    rhs.propagate_value();
    std::cout << "ConstExpr: " << rhs.emit () << std::endl;

    BinaryExpr bin_expr;
    bin_expr.set_op (BinaryExpr::Op::Shr);
    bin_expr.set_lhs (std::make_shared<ConstExpr> (lhs));
    bin_expr.set_rhs (std::make_shared<ConstExpr> (rhs));
    bin_expr.propagate_type();
    Expr::UB tmp = bin_expr.propagate_value();
    std::cout << "UB: " << tmp << std::endl;
    std::cout << "BinaryExpr: " << bin_expr.emit () << std::endl;
    std::cout << "Type: " << bin_expr.get_type_id () << std::endl;
    std::cout << "Value: " << (long long int) bin_expr.get_value () << std::endl;
*/
    return 0;
}
