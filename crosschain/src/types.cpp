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

#include <crosschain/types.h>
#include <crosschain/namegen.h>
#include <crosschain/expr.h>


using namespace crosschain;


IterVar::IterVar (rl::IntegerType::IntegerTypeID ty_)
                                    : Variable("", ty_, rl::Type::Mod::NTHG, false) {
    this->name = ItNameGen::getName();
}


IterVar::IterVar (std::string n_, rl::IntegerType::IntegerTypeID ty_)
                                    : Variable(n_, ty_, rl::Type::Mod::NTHG, false) {}


IterVar::IterVar (std::shared_ptr<rl::GenPolicy> gen_policy) : Variable("", 
            rl::IntegerType::IntegerTypeID::INT, rl::Type::Mod::NTHG, false) {
    this->name = ItNameGen::getName();

    //ScalarTypeGen scalar_type_gen (gen_policy);
    //scalar_type_gen.generate ();
    //type_id = scalar_type_gen.get_int_type_id ();
    this->type = rl::IntegerType::init (rl::IntegerType::IntegerTypeID::INT);

    rl::ModifierGen modifier_gen (gen_policy);
    modifier_gen.generate ();
    this->type->set_modifier(modifier_gen.get_modifier ());

    rl::StaticSpecifierGen static_spec_gen (gen_policy);
    static_spec_gen.generate ();
    this->type->set_is_static(static_spec_gen.get_specifier ());

    this->type->set_align(0);
    std::shared_ptr<rl::IntegerType> int_type = std::static_pointer_cast<rl::IntegerType>(type);

    this->value.int_val = int_type->get_min ();
    this->min.int_val = int_type->get_min ();
    this->max.int_val = int_type->get_max ();
}


void IterVar::setStep(std::shared_ptr<rl::Context> ctx, sll step_) {
    this->step[ctx->getScopeId()] = step_;
}


sll IterVar::getStep(std::shared_ptr<rl::Context> ctx) {
    auto search = this->step.find(ctx->getScopeId());
    if(search != this->step.end()) {
        return search->second;
    }
    return 0;
}


std::string Vector::info() {
    std::string tmp_ss;
    VecElem *p = this;
    while (p != NULL) {
        if (p->getID(0).use_count() != 0)
            tmp_ss = "[" + p->getID(0)->getExprPtr()->emit() + "]" + tmp_ss;
        p = p->getParent();
    }

    std::stringstream ss;
    ss << this->get_name() << tmp_ss << ": ";
    if (this->is_scalar())
        ss << "scalar";
    if (this->is_vector())
        ss << "vector";
        if (this->cells[0]->is_scalar())
            ss << " of scalars";
        if (this->cells[0]->is_vector())
            ss << " of vectors";
        ss << ", size = " << this->size();

    ss << ", PURPOSE: ";
    switch (this->purpose) {
        case TO_INIT: ss << "TO_INIT"; break;
        case RONLY:   ss << "RONLY"; break;
        case WONLY:   ss << "WONLY"; break;
        default: ss << "error";
    }

    ss << ", type: '" << this->base_value->get_type()->get_name();
    ss << "', val: ";

    switch(this->type_id) {
        case rl::IntegerType::IntegerTypeID::BOOL: ss << this->base_value->get_value(); break;
        case rl::IntegerType::IntegerTypeID::CHAR: ss << sll(this->base_value->get_value()); break;
        case rl::IntegerType::IntegerTypeID::UCHAR: ss << this->base_value->get_value(); break;
        case rl::IntegerType::IntegerTypeID::SHRT: ss << sll(this->base_value->get_value()); break;
        case rl::IntegerType::IntegerTypeID::USHRT: ss << this->base_value->get_value(); break;
        case rl::IntegerType::IntegerTypeID::INT: ss << sll(this->base_value->get_value()); break;
        case rl::IntegerType::IntegerTypeID::UINT: ss << this->base_value->get_value(); break;
        case rl::IntegerType::IntegerTypeID::LINT: ss << sll(this->base_value->get_value()); break;
        case rl::IntegerType::IntegerTypeID::ULINT: ss << this->base_value->get_value(); break;
        case rl::IntegerType::IntegerTypeID::LLINT: ss << sll(this->base_value->get_value()); break;
        case rl::IntegerType::IntegerTypeID::ULLINT: ss << this->base_value->get_value(); break;
        case rl::IntegerType::IntegerTypeID::MAX_INT_ID:
        default: ss << "faulty type id!";
    }

    return ss.str();
}


void VecElem::addID(IDX id_) {
    this->ids.push_back(id_);
}


std::shared_ptr<IDX> VecElem::getID (ull level) {
    if (this->ids.size() == 0)
        return std::shared_ptr<IDX>();

    if (level > this->ids.size() - 1)
        return std::make_shared<IDX>(this->ids[0]);
    return std::make_shared<IDX>(this->ids[this->ids.size() - 1 - level]);
}


Cell::Cell (VecElem *p_) {
    assert(p_ != NULL);
    this->parent = p_;
    this->purpose = this->parent->getPurpose();
}


Cell::Cell (IDX id_, VecElem *p_) {
    assert(p_ != NULL);
    this->parent = p_;
    this->purpose = this->parent->getPurpose();
    this->addID(id_);
}


std::shared_ptr<VecElem> Cell::copy() {
    std::shared_ptr<Cell> new_cell = std::make_shared<Cell>(this->parent);
    for (auto id_ : this->ids)
        new_cell->addID(id_);
    return new_cell;
}


std::shared_ptr<rl::Variable> Cell::getData() {
    assert(this->parent != NULL);
    std::string ss;

    std::shared_ptr<rl::Variable> ret = std::make_shared<rl::Variable>(*(this->parent->getData()));

    switch (this->getAccessKind ()) {
    case REGULAR : ss = ret->get_name() + "[" + this->getID(0)->getExprPtr()->emit() + "]"; break;
    case PTR     : ss = "(*(" + this->getID(0)->getExprPtr()->emit() + " + &(*" + ret->get_name() + ")))"; break;
    case AT      : ss = ret->get_name() + ".at(" + this->getID(0)->getExprPtr()->emit() + ")"; break;
    case ITERPTR : ss = "(*(" + this->getID(0)->getExprPtr()->emit() + " + " + ret->get_name() + ".begin()))"; break;
    }

    ret->set_name(ss);
    return ret;
}


Vector::Vector (ull n, VecElem *p_) {
    assert(p_ != NULL);
    this->parent = p_;
    this->type_id = this->parent->get_type_id();
    this->name = this->parent->get_name();
    this->kind = this->parent->getKind();
    this->purpose = this->parent->getPurpose();
    this->base_value = this->parent->getRawVar();
    std::vector<ull> dim_(1, n);
    this->init_multidim(dim_);
}


Vector::Vector (std::vector<ull> dim_, VecElem *p_) {
    assert(p_ != NULL);
    this->parent = p_;
    this->type_id = this->parent->get_type_id();
    this->name = this->parent->get_name();
    this->kind = this->parent->getKind();
    this->purpose = this->parent->getPurpose();
    this->base_value = this->parent->getRawVar();
    this->init_multidim(dim_);
}


Vector::Vector (ull n, std::string n_, rl::IntegerType::IntegerTypeID ty_, Kind knd_, Purpose pps_, ull val) {
    this->parent = NULL;
    this->type_id = ty_;
    this->name = n_;
    this->kind = knd_;
    this->purpose = pps_;
    this->base_value = std::make_shared<rl::Variable> (this->name, this->get_type_id(), rl::Type::Mod::NTHG, false);
    this->base_value->set_value(val);
    std::vector<ull> dim_(1, n);
    this->init_multidim(dim_);
}


Vector::Vector (std::vector<ull> dim_, std::string n_,
                                        rl::IntegerType::IntegerTypeID ty_, Kind knd_, Purpose pps_, ull val) {
    this->parent = NULL;
    this->type_id = ty_;
    this->name = n_;
    this->kind = knd_;
    this->purpose = pps_;
    this->base_value = std::make_shared<rl::Variable> (this->name, this->get_type_id(), rl::Type::Mod::NTHG, false);
    this->base_value->set_value(val);
    this->init_multidim(dim_);
}


Vector::Vector (std::shared_ptr<rl::GenPolicy> gen_policy_, Purpose pps_) {
    this->parent = NULL;
    this->gen_policy = gen_policy_;
    assert(this->gen_policy.use_count() != 0);
    rl::ScalarTypeGen scalar_type_gen (this->gen_policy);
    scalar_type_gen.generate ();
    this->type_id = scalar_type_gen.get_int_type_id ();
    this->name = VecNameGen::getName();
    this->kind = Kind(this->gen_policy->get_random_arr_kind());
    this->purpose = pps_;
    this->base_value = std::make_shared<rl::Variable> (this->name, this->get_type_id(), rl::Type::Mod::NTHG, false);
    rl::VariableValueGen var_val_gen (this->gen_policy, this->get_type_id());
    var_val_gen.generate();
    this->base_value->set_value(var_val_gen.get_value());


    ull total_cells_left = rl::rand_val_gen->get_rand_value<ull>(this->gen_policy->get_min_array_size(), 
                                                                 this->gen_policy->get_max_array_size());

    ull min_dim_size = 2;
    ull max_dim_size = this->gen_policy->get_max_array_size() / 100;
    assert(max_dim_size > 2);

    std::vector<ull> dim_;
    ull current_dim_cells = 0;
    while (total_cells_left > min_dim_size) {
        current_dim_cells = rl::rand_val_gen->get_rand_value<ull>(min_dim_size, std::min(total_cells_left, max_dim_size));
        dim_.push_back(current_dim_cells);
        total_cells_left = total_cells_left / current_dim_cells;
    }

    this->init_multidim(dim_);
}


void Vector::init_multidim(std::vector<ull> dim_) {
    assert(dim_.size() > 0);
    this->dim = dim_;
    ull mysize = dim_[0];

    if (this->getGenPolicy().use_count() != 0) {
        this->acces_probabilities.push_back(rl::Probability<AccessKind>(AccessKind::REGULAR, 
                            this->getGenPolicy()->get_access_type_score((int)AccessKind::REGULAR)));
        if (this->getKind() == Kind::C_ARR) {
            this->acces_probabilities.push_back(rl::Probability<AccessKind>(AccessKind::PTR, 
                                this->getGenPolicy()->get_access_type_score((int)AccessKind::PTR)));
        }
        else if ((this->getKind() == Kind::STD_VEC) || (this->getKind() == Kind::STD_ARR)) {
            this->acces_probabilities.push_back(rl::Probability<AccessKind>(AccessKind::AT, 
                                this->getGenPolicy()->get_access_type_score((int)AccessKind::AT)));
            this->acces_probabilities.push_back(rl::Probability<AccessKind>(AccessKind::ITERPTR, 
                                this->getGenPolicy()->get_access_type_score((int)AccessKind::ITERPTR)));
        }
    }
    else {
        this->acces_probabilities.push_back(rl::Probability<AccessKind>(AccessKind::REGULAR, 100));
    }

    if (mysize == 0) return;
    this->cells.reserve(mysize);

    if (dim_.size() == 1) {
        for (ull i = 0; i < mysize; ++i) {
            this->cells.push_back(std::make_shared<Cell>(IDX(i), this));
        }
    }
    else {
        dim_.erase(dim_.begin());
        for (ull i = 0; i < mysize; ++i) {
            this->cells.push_back(std::make_shared<Vector>(dim_, this));
            this->cells[i]->addID(IDX(i));
        }
    }
}


std::shared_ptr<VecElem> Vector::copy() {
    std::shared_ptr<Vector> new_vec = std::make_shared<Vector>(0, this->name, this->type_id, this->kind, this->purpose, this->getValue());
    new_vec->setParent(this->parent);
    for (auto id_ : this->ids)
        new_vec->addID(id_);
    new_vec->dim = this->dim;
    for (auto c_ : this->cells) {
        std::shared_ptr<VecElem> new_subcell = c_->copy();
        new_subcell->setParent(new_vec.get());
        new_vec->addCell(new_subcell);
    }
    return new_vec;
}


std::shared_ptr<rl::Variable> Vector::getData() {
    std::string ss;
    std::shared_ptr<rl::Variable> ret;

    if (this->parent != NULL) ret = std::make_shared<rl::Variable>(*(this->parent->getData()));
    else return this->getRawVar();

    switch (this->getAccessKind ()) {
    case REGULAR : ss = ret->get_name() + "[" + this->getID(0)->getExprPtr()->emit() + "]"; break;
    case PTR     : ss = "(*(" + this->getID(0)->getExprPtr()->emit() + " + &(*" + ret->get_name() + ")))"; break;
    case AT      : ss = ret->get_name() + ".at(" + this->getID(0)->getExprPtr()->emit() + ")"; break;
    case ITERPTR : ss = "(*(" + this->getID(0)->getExprPtr()->emit() + " + " + ret->get_name() + ".begin()))"; break;
    }

    ret->set_name(ss);
    return ret;
}


std::vector<std::shared_ptr<VecElem>> Vector::range (ull left, ull rght) {
    std::vector<std::shared_ptr<VecElem>> rng;
    assert(left <= rght);
    assert(rght < this->cells.size());
    for (ull i = left; i <= rght; ++i) {
        rng.push_back(this->cells[i]->copy());
    }
    return rng;
}


void Vector::register_iter (std::shared_ptr<IterVar> it) {
    for (ull i = 0; i < this->cells.size(); ++i) {
        this->cells[i]->addID(IDX(it, i));
    }
}


void Vector::register_iter (IDX idx) {
    for (sll i = 0; i < this->cells.size(); ++i) {
        IDX tmp = idx;
        tmp += i;
        this->cells[i]->addID(tmp);
    }
}


Cluster::Cluster (std::vector<std::shared_ptr<VecElem>> data, 
                  ull soverlap, 
                  std::string n_) : Vector(0, n_, data[0]->get_type_id(), data[0]->getKind(), data[0]->getPurpose(), data[0]->getValue()) {
    this->cells = data;
    for (ull i = 0; i < data.size(); ++i)
        this->overlaps.push_back(1);

    assert(soverlap <= data.size());
    for (ull i = soverlap; i < data.size() - soverlap; ++i)
        this->overlaps[i] = 0;
}


std::shared_ptr<IDX> Cluster::getID (ull level) {
    return this->cells[0]->getID(level);
}
