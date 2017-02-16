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

#include <crosschain/stmt.h>
#include <crosschain/namegen.h>

using namespace crosschain;


RInitStmtNVC::RInitStmtNVC(std::shared_ptr<rl::Context> ctx, std::vector<std::shared_ptr<rl::Variable>> in, 
                                                                           std::shared_ptr<rl::Variable> out) {
    assert(out.use_count() != 0);
    rl::ArithStmtGen arith_stmt_gen(ctx, in, std::shared_ptr<rl::Variable>());
    arith_stmt_gen.generate();
    this->lhs = out;
    this->rhs = arith_stmt_gen.get_expr();

    // Normalize rhs to 'out' value
    std::shared_ptr<rl::VarUseExpr> lhs_expr = std::make_shared<rl::VarUseExpr>(this->lhs);
    lhs_expr->propagate_type();
    assert(lhs_expr->propagate_value() == rl::Expr::NoUB && "UB? In my code?!");

    std::shared_ptr<rl::ConstExpr> lhs_const = std::make_shared<rl::ConstExpr>(lhs_expr->get_int_type_id(), lhs_expr->get_value());
    lhs_const->propagate_type();
    assert(lhs_const->propagate_value() == rl::Expr::NoUB && "UB? In my code?!");

    std::shared_ptr<rl::ConstExpr> rhs_const = std::make_shared<rl::ConstExpr>(this->rhs->get_int_type_id(), this->rhs->get_value());
    rhs_const->propagate_type();
    assert(rhs_const->propagate_value() == rl::Expr::NoUB && "UB? In my code?!");

    this->rhs = std::make_shared<rl::BinaryExpr>(rl::BinaryExpr::Op::Sub, this->rhs, rhs_const);
    this->rhs->propagate_type();
    assert(this->rhs->propagate_value() == rl::Expr::NoUB && "UB? In my code?!");

    this->rhs = std::make_shared<rl::BinaryExpr>(rl::BinaryExpr::Op::Add, this->rhs, lhs_const);
    this->rhs->propagate_type();
    assert(this->rhs->propagate_value() == rl::Expr::NoUB && "UB? In my code?!");

    this->result = std::make_shared<rl::ExprStmt>(std::make_shared<rl::AssignExpr>(lhs_expr, this->rhs));
}


std::string RInitStmtNVC::info() {
    std::stringstream ss;
    ss << this->lhs->get_name() << " = ";
    if(this->rhs->get_type_sign())
        ss << sll(this->rhs->get_value());
    else
        ss << this->rhs->get_value();
    ss << ";";
    return ss.str();
}


IterDeclStmt::IterDeclStmt (std::shared_ptr<rl::Context> ctx, rl::IntegerType::IntegerTypeID ty_, sll step_) {
    this->data = std::make_shared<IterVar>(ty_);
    this->data->setStep(ctx, step_);
}


IterDeclStmt::IterDeclStmt (std::shared_ptr<rl::Context> ctx, rl::IntegerType::IntegerTypeID ty_, sll step_, 
                                                            std::shared_ptr<IDX> in_) {
    this->data = std::make_shared<IterVar>(ty_);
    this->data->setStep(ctx, step_);
    this->init = in_;
}


IterDeclStmt::IterDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, rl::IntegerType::IntegerTypeID ty_, sll step_) {
    this->data = std::make_shared<IterVar>(n_, ty_);
    this->data->setStep(ctx, step_);
}


IterDeclStmt::IterDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, rl::IntegerType::IntegerTypeID ty_, sll step_, 
                                                            std::shared_ptr<IDX> in_) {
    this->data = std::make_shared<IterVar>(n_, ty_);
    this->data->setStep(ctx, step_);
    this->init = in_;
}


IterDeclStmt::IterDeclStmt (std::shared_ptr<rl::Context> ctx, sll step_) {
    this->data = std::make_shared<IterVar>(ctx->get_self_gen_policy());
    this->data->setStep(ctx, step_);
    this->init = std::make_shared<IDX>(rl::rand_val_gen->get_rand_value<sll>(0, 100));
}


IterDeclStmt::IterDeclStmt (std::shared_ptr<rl::Context> ctx, sll step_, std::shared_ptr<IDX> in_) {
    this->data = std::make_shared<IterVar>(ctx->get_self_gen_policy());
    this->data->setStep(ctx, step_);
    this->init = in_;
}


std::shared_ptr<rl::Expr> IterDeclStmt::getStepExpr (std::shared_ptr<rl::Context> ctx) {
    std::shared_ptr<rl::BinaryExpr> step_expr = std::make_shared<rl::BinaryExpr>();
    std::shared_ptr<rl::VarUseExpr> lhs = std::make_shared<rl::VarUseExpr>();
    lhs->set_variable(this->data);
    std::shared_ptr<rl::ConstExpr>  rhs = std::make_shared<rl::ConstExpr>();
    rhs->set_type(rl::IntegerType::IntegerTypeID::INT);
    rhs->set_data(this->data->getStep(ctx));
    step_expr->set_op(rl::BinaryExpr::Add);
    step_expr->set_lhs(lhs);
    step_expr->set_rhs(rhs);
    return step_expr;
}


std::string IterDeclStmt::IterDeclStmt::emit (std::string offset) {
    std::stringstream ss;
    ss << offset;
    if (this->is_extern) ss << "extern ";
    ss << this->data->get_type()->get_name() << " " << this->data->get_name();

    if (this->init.use_count() != 0)
        ss << " = " << this->get_init()->emit();
    ss << ";";
    return ss.str();
}


VectorDeclStmt::VectorDeclStmt (std::shared_ptr<rl::Context> ctx, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, ull num_) {
    this->context = ctx;
    rl::VariableValueGen var_val_gen (ctx->get_self_gen_policy(), ty_);
    var_val_gen.generate();
    this->data = std::make_shared<Vector>(num_, VecNameGen::getName(), ty_, knd_, VecElem::Purpose::TO_INIT, var_val_gen.get_value());
}


VectorDeclStmt::VectorDeclStmt (std::shared_ptr<rl::Context> ctx, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, std::vector<ull> dim) {
    this->context = ctx;
    rl::VariableValueGen var_val_gen (ctx->get_self_gen_policy(), ty_);
    var_val_gen.generate();
    this->data = std::make_shared<Vector>(dim, VecNameGen::getName(), ty_, knd_, VecElem::Purpose::TO_INIT, var_val_gen.get_value());
}


VectorDeclStmt::VectorDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, ull num_) {
    this->context = ctx;
    rl::VariableValueGen var_val_gen (ctx->get_self_gen_policy(), ty_);
    var_val_gen.generate();
    this->data = std::make_shared<Vector>(num_, n_, ty_, knd_, VecElem::Purpose::TO_INIT, var_val_gen.get_value());
}


VectorDeclStmt::VectorDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, std::vector<ull> dim) {
    this->context = ctx;
    rl::VariableValueGen var_val_gen (ctx->get_self_gen_policy(), ty_);
    var_val_gen.generate();
    this->data = std::make_shared<Vector>(dim, n_, ty_, knd_, VecElem::Purpose::TO_INIT, var_val_gen.get_value());
}


VectorDeclStmt::VectorDeclStmt (std::shared_ptr<rl::Context> ctx) {
    this->context = ctx;
    this->data = std::make_shared<Vector>(ctx->get_self_gen_policy(), VecElem::Purpose::TO_INIT);
}


std::shared_ptr<rl::Stmt> VectorDeclStmt::getInitStmt() {
    // For the external init block any context will do
    rl::GenPolicy gp;
    gp.set_lue_prob(0);
    std::shared_ptr<rl::Context> ctx = std::make_shared<rl::Context>(gp, 
                                                                     std::shared_ptr<rl::Stmt>(), 
                                                                     std::shared_ptr<rl::Context>());
    if ((this->data->getKind() == VecElem::Kind::C_ARR) || (this->data->getKind() == VecElem::Kind::STD_ARR)) {
        std::shared_ptr<ForEachStmt> ret = std::make_shared<ForEachStmt>(this->get_data());
        ret->add_single_stmt(std::make_shared<rl::ExprStmt>
            (std::make_shared<rl::AssignExpr>
            (std::make_shared<rl::VarUseExpr>
            (ret->getVar()), std::make_shared<rl::ConstExpr>
            (std::static_pointer_cast<rl::IntegerType>
            (ret->getVar()->get_type())->get_int_type_id(), ret->getVar()->get_value()))));
        return ret;
    }
    if ((this->data->getKind() == VecElem::Kind::STD_VEC) || (this->data->getKind() == VecElem::Kind::STD_VARR)) {
        std::shared_ptr<IterDeclStmt> it = std::make_shared<IterDeclStmt>(ctx, rl::IntegerType::IntegerTypeID::ULLINT, 1, std::make_shared<IDX>(0));
        std::shared_ptr<HeaderStmt> ret = std::make_shared<HeaderStmt>(ctx, it, 1); // Do not have 'scope' class yet
        std::shared_ptr<HeaderStmt> hdr = ret;
        std::string var_name = this->data->get_name();

        for (ull i = 0; i < this->data->dim.size() - 1; ++i) {
            std::stringstream ss;
            ss << var_name << ".resize(" << this->data->dim[i] << ");";
            hdr->add(std::make_shared<rl::StubStmt>(ss.str()));
            it = std::make_shared<IterDeclStmt>(ctx, rl::IntegerType::IntegerTypeID::ULLINT, 1, std::make_shared<IDX>(0));
            std::shared_ptr<HeaderStmt> new_hdr = std::make_shared<HeaderStmt>(ctx, it, this->data->dim[i]);
            hdr->add(new_hdr);
            hdr = new_hdr;
            var_name = var_name + "[" + it->get_name() + "]";
        }

        std::stringstream ss;
        ss << var_name << ".resize(" << this->data->dim.back() << ", ";
        rl::ConstExpr init_val = rl::ConstExpr(data->get_type_id(), data->get_value());
        ss << init_val.emit() << ");";
        hdr->add(std::make_shared<rl::StubStmt>(ss.str()));
        return ret;
    }

    return std::make_shared<rl::StubStmt>("Unreachable Error!");
}


std::string VectorDeclStmt::emit (std::string offset) {
    std::stringstream ss;
    ss << offset;
    if (this->is_extern) ss << "extern ";

    if (this->data->getKind() == VecElem::Kind::C_ARR) {
        ss << this->data->get_type()->get_name()
            << " " << this->data->get_name();
        for (ull i = 0; i < this->data->dim.size(); ++i)
            ss << "[" << this->data->dim[i] << "]";
    }
    if (this->data->getKind() == VecElem::Kind::STD_VEC) {
        for (ull i = 0; i < this->data->dim.size(); ++i)
            ss << "std::vector<";
        ss << this->data->get_type()->get_name();
        for (ull i = 0; i < this->data->dim.size(); ++i)
            ss << " >";
        ss << " " << this->data->get_name();
    }
    if (this->data->getKind() == VecElem::Kind::STD_VARR) {
        for (ull i = 0; i < this->data->dim.size(); ++i)
            ss << "std::valarray<";
        ss << this->data->get_type()->get_name();
        for (ull i = 0; i < this->data->dim.size(); ++i)
            ss << " >";
        ss << " " << this->data->get_name();
    }
    if (this->data->getKind() == VecElem::Kind::STD_ARR) {
        for (ull i = 0; i < this->data->dim.size(); ++i)
            ss << "std::array<";
        ss << this->data->get_type()->get_name();
        for (sll i = this->data->dim.size() - 1; i >= 0; i--)
            ss << ", " <<  this->data->dim[i] << ">";
        ss << " " << this->data->get_name();
    }

    ss << ";";
    return ss.str();
}


HeaderStmt::HeaderStmt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<IterDeclStmt> it, std::shared_ptr<IDX> start_e, ull tripcount) {
    this->id = rl::Node::NodeID::LHDR;
    // i = start
    this->start = std::make_shared<rl::AssignExpr>(std::make_shared<rl::VarUseExpr>(it->get_data()), start_e->getExprPtr());
    // i = i + step
    this->step = std::make_shared<rl::AssignExpr>(std::make_shared<rl::VarUseExpr>(it->get_data()), 
                                                                                   it->getStepExpr(ctx));

    // i < start + tc * step
    std::shared_ptr<rl::Expr> end_expr = std::make_shared<rl::ConstExpr>(rl::IntegerType::IntegerTypeID::INT, tripcount * it->get_data()->getStep(ctx));

    if (ctx->get_self_gen_policy()->do_loop_unknown_end()) {
        std::shared_ptr<rl::Variable> var = std::make_shared<rl::Variable> ("le_" + SclNameGen::getName(), 
                                                    rl::IntegerType::IntegerTypeID::INT, rl::Type::Mod::NTHG, false);
        var->set_value(tripcount * it->get_data()->getStep(ctx));
        ctx->get_extern_inp_sym_table()->add_variable(var);
        end_expr = std::make_shared<rl::VarUseExpr>(var);
    }

    this->exit = std::make_shared<rl::BinaryExpr>
        (rl::BinaryExpr::Lt, std::make_shared<rl::VarUseExpr>(it->get_data()), std::make_shared<rl::BinaryExpr>
        (rl::BinaryExpr::Op::Add, start_e->getExprPtr(), end_expr));
}


HeaderStmt::HeaderStmt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<IterDeclStmt> it, ull tripcount) {
    this->id = rl::Node::NodeID::LHDR;

    // int i = start
    this->decl = it;

    // i = i + step
    this->step = std::make_shared<rl::AssignExpr>(std::make_shared<rl::VarUseExpr>(it->get_data()), 
                                                                                   it->getStepExpr(ctx));
    // i < start + tc * step
    std::shared_ptr<rl::Expr> end_expr = std::make_shared<rl::ConstExpr>(rl::IntegerType::IntegerTypeID::INT, tripcount * it->get_data()->getStep(ctx));

    if (ctx->get_self_gen_policy()->do_loop_unknown_end()) {
        std::shared_ptr<rl::Variable> var = std::make_shared<rl::Variable> ("le_" + SclNameGen::getName(), 
                                                    rl::IntegerType::IntegerTypeID::INT, rl::Type::Mod::NTHG, false);
        var->set_value(tripcount * it->get_data()->getStep(ctx));
        ctx->get_extern_inp_sym_table()->add_variable(var);
        end_expr = std::make_shared<rl::VarUseExpr>(var);
    }

    assert(this->decl->get_init().use_count() != 0);
    this->exit = std::make_shared<rl::BinaryExpr>
        (rl::BinaryExpr::Op::Lt, std::make_shared<rl::VarUseExpr>(it->get_data()), std::make_shared<rl::BinaryExpr>
        (rl::BinaryExpr::Op::Add, this->decl->get_init(), end_expr));
}


std::string HeaderStmt::emit (std::string offset) {
    std::stringstream ss;
    ss << offset << "for (";
    if (this->start.use_count() != 0)
        ss << this->start->emit() << ";";
    else {
        assert(this->decl->get_init().use_count() != 0);
        ss << this->decl->emit();
    }
    ss << " " << this->exit->emit();
    ss << "; " << this->step->emit();
    ss << ")";
    if (this->module.size() == 1) ss << "\n";
    else ss << " {\n";

    for (auto e : this->module) {
        if ((e->get_id() == rl::Node::NodeID::CMNT) && (e->is_dead()))
            continue;
        ss << e->emit(offset + "    ") << "\n";
    }

    if (this->module.size() != 1) ss << offset << "}";
    return ss.str();
}


ForEachStmt::ForEachStmt(std::shared_ptr<Vector> v) {
    this->vector = v;
    assert(this->vector.use_count() != 0);
    std::stringstream v_name;
    v_name << v->get_name();
    for(ull i = 0; i < this->vector->dim.size(); ++i)
        v_name << "[" << v->get_name() << "_it" << i << "]";
    this->var = std::make_shared<rl::Variable>(*(this->vector->getData()));
    this->var->set_name(v_name.str());
}


std::string ForEachStmt::emit (std::string offset) {
    assert(this->todostmt.use_count() != 0);

    std::stringstream ss;
    std::stringstream ss_offset;

    ss_offset << offset;
    for (ull i = 0; i < this->vector->dim.size(); ++i) {
        std::string i_name = this->vector->get_name() + "_it";
        ss << ss_offset.str() << "for(unsigned long long int " << i_name << i
           << " = 0; " << i_name << i << " < " << this->vector->dim[i]
           << "; ++" << i_name << i << ")\n";
        ss_offset << "    ";
    }

    ss << this->todostmt->emit(ss_offset.str());
    return ss.str();
}
