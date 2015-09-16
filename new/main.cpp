#include "type.h"
#include "variable.h"
#include "node.h"

int main () {
    std::shared_ptr<Type> type;
    type = Type::init (Type::TypeID::UINT);
    type->dbg_dump();

    Variable var = Variable("i", Type::TypeID::UINT, Variable::Mod::NTHNG);
    var.set_modifier (Variable::Mod::CONST);
    var.set_value (10);
    var.set_min (5);
    var.set_max (50);
    var.dbg_dump ();

    Array arr = Array ("a", Type::TypeID::UINT, Variable::Mod::CONST, 20, 
                           Array::Ess::STD_ARR);
    arr.dbg_dump();

    VarUseExpr var_use;
    var_use.set_variable (std::make_shared<Variable> (var));
    std::cout << "VarUseExpr: " << var_use.emit () << std::endl;

    VarUseExpr var_arr_use;
    var_arr_use.set_variable (std::make_shared<Variable> (arr));
    std::cout << "VarUseExpr arr: " << var_arr_use.emit () << std::endl;

    AssignExpr assign;
    assign.set_to (std::make_shared<VarUseExpr> (var_arr_use));
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
    return 0;
}
