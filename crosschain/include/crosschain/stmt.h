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

#ifndef STMT_H
#define STMT_H

#include <crosschain/typedefs.h>
#include <crosschain/types.h>
#include <crosschain/expr.h>

#include <node.h>

namespace crosschain {


class RInitStmtNVC : public rl::Stmt {
protected:
    std::shared_ptr<rl::Stmt> result;
    std::shared_ptr<rl::Expr> rhs;
    std::shared_ptr<rl::Variable> lhs;

public:
    RInitStmtNVC(std::shared_ptr<rl::Context> ctx, std::vector<std::shared_ptr<rl::Variable>> in, std::shared_ptr<rl::Variable> out);
    virtual std::string emit (std::string offset = "") {return this->result->emit(offset); }
    virtual std::string info();
};


class CommentStmt : public rl::Stmt {
protected:
    std::string comment;

public:
    CommentStmt(std::shared_ptr<rl::Context> ctx, std::string s_, ull level = 0) {
        this->id = rl::Node::NodeID::CMNT;
        if (ctx->get_verbose_level() > level) {this->set_dead(); return;}
        if (s_ == "") this->comment = "";
        else this->comment = "/* " + s_ + " */";
    }

    virtual std::string emit(std::string offset = "") {
        sll current_pos = 0;
        while ((current_pos < this->comment.size()) && (current_pos != std::string::npos)) {
            current_pos ++;
            current_pos = this->comment.find_first_of("\n", current_pos);
            this->comment.insert(current_pos + 1, offset);
        }
        return this->comment;
    }
};


class IterDeclStmt : public rl::DeclStmt {
private:
    std::shared_ptr<IterVar> data;
    std::shared_ptr<IDX> init;

public:
    IterDeclStmt (std::shared_ptr<rl::Context> ctx, rl::IntegerType::IntegerTypeID ty_, sll step_);
    IterDeclStmt (std::shared_ptr<rl::Context> ctx, rl::IntegerType::IntegerTypeID ty_, sll step_, std::shared_ptr<IDX> in_);

    IterDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, rl::IntegerType::IntegerTypeID ty_, sll step_);
    IterDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, rl::IntegerType::IntegerTypeID ty_, sll step_, std::shared_ptr<IDX> in_);

    IterDeclStmt (std::shared_ptr<rl::Context> ctx, sll step_);
    IterDeclStmt (std::shared_ptr<rl::Context> ctx, sll step_, std::shared_ptr<IDX> in_);

    std::shared_ptr<IterVar> get_data () {return this->data; }
    std::shared_ptr<rl::Expr> get_init () {return this->init->getExprPtr(); }
    std::shared_ptr<IDX> get_init_idx () {return this->init; }
    virtual std::string get_name() {return this->data->get_name(); }

    virtual std::shared_ptr<rl::Expr> getStepExpr (std::shared_ptr<rl::Context> ctx);
    virtual std::string emit (std::string offset = "");
};


class VectorDeclStmt : public rl::DeclStmt {
private:
    std::shared_ptr<Vector> data;
    std::shared_ptr<rl::Expr> init;
    std::shared_ptr<rl::Context> context;

public:
    VectorDeclStmt (std::shared_ptr<rl::Context> ctx, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, ull num_);
    VectorDeclStmt (std::shared_ptr<rl::Context> ctx, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, std::vector<ull> dim);

    VectorDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, ull num_);
    VectorDeclStmt (std::shared_ptr<rl::Context> ctx, std::string n_, VecElem::Kind knd_, rl::IntegerType::IntegerTypeID ty_, std::vector<ull> dim);

    VectorDeclStmt (std::shared_ptr<rl::Context> ctx);

    void setPurpose(VecElem::Purpose p_) {this->data->setPurpose(p_);}
    std::shared_ptr<Vector> get_data () {return this->data; }

    virtual std::shared_ptr<rl::Stmt> getInitStmt();
    virtual std::string emit (std::string offset = "");
};


class HeaderStmt : public rl::Stmt {
protected:
    std::vector<std::shared_ptr<rl::Stmt>> module;

private:
    std::shared_ptr<IterDeclStmt> decl;
    std::shared_ptr<rl::AssignExpr> start;
    std::shared_ptr<rl::Expr> exit;
    std::shared_ptr<rl::Expr> step;

public:
    HeaderStmt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<IterDeclStmt> it, ull tripcount);
    HeaderStmt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<IterDeclStmt> it, 
                std::shared_ptr<IDX> start_e, ull tripcount);

    virtual void add (std::shared_ptr<rl::Stmt> e) {
        this->module.push_back(e);
    }

    virtual std::string emit (std::string offset = "");
};


class ForEachStmt : public rl::Stmt {
protected:
    std::shared_ptr<rl::Variable> var;
    std::shared_ptr<rl::Stmt> todostmt;
    std::shared_ptr<Vector> vector;

public:
    ForEachStmt(std::shared_ptr<Vector> v);
    virtual std::shared_ptr<rl::Variable> getVar () {return this->var; }
    virtual void add_single_stmt(std::shared_ptr<rl::Stmt> s_) {this->todostmt = s_;}
    virtual std::string emit (std::string offset = "");
};
}

#endif
