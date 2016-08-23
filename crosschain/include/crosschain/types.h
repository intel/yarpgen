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

#ifndef TYPES_H
#define TYPES_H

#include <crosschain/typedefs.h>
#include <variable.h>
#include <generator.h>


namespace crosschain {

class IDX;

class IterVar : public rl::Variable {
protected:
    std::map<ull, sll> step;

public:
    IterVar (rl::IntegerType::IntegerTypeID ty_);
    IterVar (std::string n_, rl::IntegerType::IntegerTypeID ty_);
    IterVar (std::shared_ptr<rl::GenPolicy> gen_policy);
    virtual void setStep(std::shared_ptr<rl::Context> ctx, sll step_);
    virtual sll getStep(std::shared_ptr<rl::Context> ctx);
};


class VecElem {
public:
    enum Purpose {NONE, TO_INIT, RONLY, WONLY};
    enum Kind {C_ARR, STD_VEC, STD_VARR, STD_ARR};
    enum AccessKind {REGULAR, AT, PTR, ITERPTR};

protected:
    // Some data is common for every element in array (like type or name). So remember who is your
    // parent to restore the info and at the same time save some memory.
    VecElem *parent;

    // The list of indexes to access current cell (like [i+1], [j], [k-234], ...)
    std::vector<IDX> ids;

    // The kind of array we are ging ti emit
    // C_ARR: int a[42] ... - c - style array
    // STD_VEC:  std::vector
    // STD_VARR: std::valarray
    // STD::ARR: std::array
    Kind kind;

    // This ... solution will help to determine the purpose this vector exists for.
    // This is a major drawback of our speculation, that all input vectors have the same value.
    // Uninitialized read is UB. The TO_INIT vector is a vector bound to undergo *full* initialization,
    // meaning that *all* cells are set with a specific value determined earlier at the creation of the vector.
    // After vector is initialized, it is set to be RONLY, so that noone mess with the data. RONLY cells
    // have the same value for all cells. The WONLY vecs are like TO_INIT, but there is no guarantee
    // that all cells are initialized with the same values. No cell shall be left uninitialized (FIXME?)
    // or in the end we cannot check the final result.
    Purpose purpose;

public:
    virtual Purpose getPurpose() {return this->purpose;}
    virtual Kind getKind() {return this->kind;}
    virtual void setPurpose(Purpose pps_) {this->purpose = pps_;}

    virtual std::string info() {return "none";}
    virtual bool is_vector() = 0;
    virtual bool is_scalar() = 0;
    virtual bool equals(std::shared_ptr<VecElem> v) {return (this->get_name() == v->get_name());}
    virtual std::shared_ptr<VecElem> copy() = 0;
    virtual std::shared_ptr<rl::Variable> getData() = 0;

    virtual std::shared_ptr<IDX> getID (ull level = 0);
    virtual void addID(IDX id_);

    virtual AccessKind getAccessKind () {
        assert(this->parent != NULL);
        return this->parent->getAccessKind();
    }

    virtual std::shared_ptr<rl::GenPolicy> getGenPolicy () {
        assert(this->parent != NULL);
        return this->parent->getGenPolicy();
    }

    virtual rl::IntegerType::IntegerTypeID get_type_id () {
        assert(this->parent != NULL);
        return this->parent->get_type_id();
    }

    virtual std::shared_ptr<rl::Type> get_type () {
        assert(this->parent != NULL);
        return this->parent->get_type();
    }

    virtual std::string get_name () {
        assert(this->parent != NULL);
        return this->parent->get_name();
    }

    virtual std::shared_ptr<rl::Variable> getRawVar() {
        assert(this->parent != NULL);
        return this->parent->getRawVar();
    }

    virtual uint64_t get_value() {
        assert(this->parent != NULL);
        return this->parent->get_value();
    }

    virtual ull getValue() {return this->getRawVar()->get_value();}

    virtual VecElem* getParent() {return this->parent;}
    virtual void setParent(VecElem *p_) {this->parent = p_;}
};


class Cell : public VecElem {
public:
    Cell (VecElem *p_);
    Cell (IDX id_, VecElem *p_);

    virtual std::shared_ptr<VecElem> copy() override;
    virtual std::shared_ptr<rl::Variable> getData() override;

    virtual bool is_vector() override {return false;}
    virtual bool is_scalar() override {return true;}
};


/** \brief A helper class to ease up passing vector ranges around.
 */
struct RANGE {
public:
    ull l;
    ull r;
    RANGE(){}
    RANGE(ull l_, ull r_) : l(l_), r(r_) {}
};

class Vector : public VecElem {
protected:
    std::string name;
    rl::IntegerType::IntegerTypeID type_id;
    std::vector<std::shared_ptr<VecElem>> cells;

    // This variable stores values for all vector cells
    // If <purpose> is set to TO_INIT, the value equals to the desired vector value;
    // if <purpose> is set to RONLY, the correct init code have been emitted for all
    // cells in vector; if <purpose> is WONLY, the value does not matter
    std::shared_ptr<rl::Variable> base_value;
    std::shared_ptr<rl::GenPolicy> gen_policy;

    std::vector<rl::Probability<AccessKind>> acces_probabilities;

public:
    std::vector<ull> dim;

    Vector (ull n, VecElem *p_);
    Vector (std::vector<ull> dim, VecElem *p_);
    Vector (ull n, std::string n_, rl::IntegerType::IntegerTypeID ty_, Kind knd_, Purpose pps_, ull val);
    Vector (std::vector<ull> dim, std::string n_, rl::IntegerType::IntegerTypeID ty_, Kind knd_, Purpose pps_, ull val);
    Vector (std::shared_ptr<rl::GenPolicy> gen_policy_, Purpose pps_);

    virtual rl::IntegerType::IntegerTypeID get_type_id () override {return this->type_id;}
    virtual std::shared_ptr<rl::Type> get_type () override {
        return rl::IntegerType::init(this->type_id);
    }
    virtual std::string get_name () override {return this->name;}

    virtual std::shared_ptr<VecElem> copy() override;
    virtual std::shared_ptr<rl::Variable> getData() override;

    virtual bool is_vector() override {return true;}
    virtual bool is_scalar() override {return false;}

    virtual std::string info() override;
    virtual void addCell(std::shared_ptr<VecElem> c_) {this->cells.push_back(c_);}

    virtual std::vector<std::shared_ptr<VecElem>> range (ull left, ull rght);
    virtual std::vector<std::shared_ptr<VecElem>> range (RANGE rng) {
        return this->range(rng.l, rng.r);
    }

    virtual void setPurpose(Purpose pps_) override {
        for(auto cell : this->cells) cell->setPurpose(pps_);
        this->purpose = pps_;
    }

    virtual AccessKind getAccessKind () override {
        if(this->parent != NULL) return this->parent->getAccessKind();
        return rl::rand_val_gen->get_rand_id(this->acces_probabilities);
    }

    virtual uint64_t get_value() override {
        assert(this->base_value.use_count() != 0);
        return this->base_value->get_value();
    }

    virtual std::shared_ptr<rl::GenPolicy> getGenPolicy () override {
        return this->gen_policy;
    }

    virtual void register_iter (std::shared_ptr<IterVar> it);
    virtual void register_iter (IDX idx);
    virtual ull size() {
        return this->cells.size();
    }

    virtual std::shared_ptr<rl::Variable> getRawVar() override {
        assert(this->base_value.use_count() != 0);
        return this->base_value;
    }

    virtual std::shared_ptr<VecElem>& operator[](ull idx) {
        assert(idx < this->size());
        return this->cells[idx];
    }

private:
    void init_multidim(std::vector<ull> dim);
};


class Cluster : public Vector {
public:
    std::vector<ull> overlaps;
    Cluster (std::vector<std::shared_ptr<VecElem>> data, 
             ull soverlap = 0, 
             std::string n_ = "noname cluster");

    virtual std::shared_ptr<IDX> getID (ull level = 0) override;
};
}

#endif
