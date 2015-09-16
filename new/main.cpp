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

    return 0;
}
