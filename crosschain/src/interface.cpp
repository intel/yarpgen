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

#include <crosschain/interface.h>

using namespace crosschain;


IfceIt::IfceIt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<Vector> v, ull A, ull ovp) : Ifce(IFCEIT) {
    assert(v.use_count() != 0);
    assert(v->size() > 0);
    assert(A > 0);
    sll step = A - ovp;
    assert(step != 0);

    this->tripcount = 0;
    if (v->size() >= A)
        this->tripcount = 1 + (v->size() - A) / std::abs(step);

    std::vector<RANGE> rngs;
    std::vector<RANGE> outliers;
    for (ull i = 0; i < this->tripcount; ++i) {
        rngs.push_back(RANGE(i*step, i*step + A - 1));
    }

    if (rngs.size() == 0) {
        outliers.push_back(RANGE(0, v->size() - 1));
    }
    else {
        if (rngs[0].l > 0)
            outliers.push_back(RANGE(0, rngs[0].l - 1));
        for (sll i = 0; i < rngs.size() - 1; ++i)
            if(rngs[i].r < rngs[i+1].l - 1)
                outliers.push_back(RANGE(rngs[i].r + 1, rngs[i+1].l - 1));
        if (v->size() - 1 >= rngs.back().r + 1)
            outliers.push_back(RANGE(rngs.back().r + 1, v->size() - 1));
    }

    for (auto rng : outliers) {
        std::shared_ptr<Cluster> cl = std::make_shared<Cluster>
                                  (v->range(rng), 0, v->get_name());
        assert(cl->size() > 0);
        if (cl->size() == 1) this->vecs.push_back((*cl)[0]);
        else this->vecs.push_back(cl);
    }

    if (rngs.size() == 0) return;

    std::shared_ptr<IDX> start = (*v)[rngs[0].l]->getID(0); // v[i, i+1, i+2 ..] => start = i
    this->it = std::make_shared<IterDeclStmt>(ctx, rl::IntegerType::IntegerTypeID::INT, step, start);

    std::shared_ptr<Cluster> cl = std::make_shared<Cluster>(v->range(rngs[0]), ovp, v->get_name());
    assert(cl->size() > 0);
    cl->register_iter(this->it->get_data());
    if (cl->size() == 1) this->vecs.push_back((*cl)[0]);
    else this->vecs.push_back(cl);
}


IfceIt::IfceIt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<Vector> v, std::shared_ptr<IterDeclStmt> it_, ull tc_) : Ifce(IFCEIT) {
    assert(v.use_count() != 0);
    assert(v->size() > 0);
    assert(it_.use_count() != 0);
    assert(tc_ != 0);
    this->tripcount = tc_;
    this->it = it_;
    sll step = std::abs(this->it->get_data()->getStep(ctx));
    if (v->size() <= step*(tc_ - 1)) {
        if (v->size() == 1) this->vecs.push_back((*v)[0]);
        else this->vecs.push_back(v);
        return;
    }
    ull maxA = v->size() - step*(tc_ - 1);
    ull maxovp = (maxA < step) ? 0 : maxA - step;

    maxA = maxA - maxovp;                    // no cross deps

    // pick out A: (A = 1 .. maxA)
    ull A = maxA;
    ull maxShift = maxA - A;

    // pick out shift: (shift = 0 .. maxShift)
    ull shift = maxShift;

    // calculate overlap
    ull ovp = (A < step) ? 0 : A - step;
    assert(ovp == 0);

    std::vector<RANGE> rngs;
    std::vector<RANGE> outliers;
    for (ull i = 0; i < this->tripcount; ++i) {
        rngs.push_back(RANGE(maxShift + i*step, maxShift + i*step + A - 1));
    }

    // Need to process every cell
    if (v->getPurpose() == VecElem::Purpose::TO_INIT) {
        if (rngs.size() == 0) {
            outliers.push_back(RANGE(0, v->size() - 1));
        }
        else {
            if (rngs[0].l > 0)
                outliers.push_back(RANGE(0, rngs[0].l - 1));
            for (sll i = 0; i < rngs.size() - 1; ++i)
                if(rngs[i].r < rngs[i+1].l - 1)
                    outliers.push_back(RANGE(rngs[i].r + 1, rngs[i+1].l - 1));
            if (v->size() - 1 >= rngs.back().r + 1)
                outliers.push_back(RANGE(rngs.back().r + 1, v->size() - 1));
        }

        for (auto rng : outliers) {
            std::shared_ptr<Cluster> cl = std::make_shared<Cluster>
                                      (v->range(rng), 0, v->get_name());
            assert(cl->size() > 0);
            if (cl->size() == 1) this->vecs.push_back((*cl)[0]);
            else this->vecs.push_back(cl);
        }
    }

    assert(rngs.size() > 0);

    std::shared_ptr<Cluster> cl = std::make_shared<Cluster>(v->range(rngs[0]), ovp, v->get_name());
    assert(cl->size() > 0);

    IDX tmp(this->it->get_data());
    tmp -= *(this->it->get_init_idx());
    tmp += *((*cl)[0]->getID(0));
    tmp.clearup_tree(); // Remove redundant empty subexpressions

    cl->register_iter(tmp);

    // do the lowering: if our size is 1, then we loose current dimension
    if (cl->size() == 1) this->vecs.push_back((*cl)[0]);
    else this->vecs.push_back(cl);
}


IfcePart::IfcePart (std::shared_ptr<rl::Context> ctx, std::shared_ptr<Vector> v, ull A, ull ovp) : Ifce(IFCEIT) {
    assert(v.use_count() != 0);
    assert(v->size() > 0);
    assert(A > 0);
    sll step = A - ovp;
    assert(step != 0);

    ull tripcount = 0;
    if (v->size() >= A)
        tripcount = 1 + (v->size() - A) / std::abs(step);

    std::vector<RANGE> rngs;
    std::vector<RANGE> outliers;
    for (ull i = 0; i < tripcount; ++i) {
        rngs.push_back(RANGE(i*step, i*step + A - 1));
    }

    if (rngs.size() == 0) {
        outliers.push_back(RANGE(0, v->size() - 1));
    }
    else {
        if (rngs[0].l > 0)
            outliers.push_back(RANGE(0, rngs[0].l - 1));
        for (sll i = 0; i < rngs.size() - 1; ++i)
            if(rngs[i].r < rngs[i+1].l - 1)
                outliers.push_back(RANGE(rngs[i].r + 1, rngs[i+1].l - 1));
        if (v->size() - 1 >= rngs.back().r + 1)
            outliers.push_back(RANGE(rngs.back().r + 1, v->size() - 1));
    }

    for (auto rng : outliers) {
        std::shared_ptr<Cluster> cl = std::make_shared<Cluster>
                                  (v->range(rng), 0, v->get_name());
        assert(cl->size() > 0);
        if (cl->size() == 1) this->vecs.push_back((*cl)[0]);
        else this->vecs.push_back(cl);
    }

    if (rngs.size() == 0) return;

    for (auto rng : rngs) {
        std::shared_ptr<Cluster> cl = std::make_shared<Cluster>(v->range(rng), ovp, v->get_name());
        assert(cl->size() > 0);
        if (cl->size() == 1) this->vecs.push_back((*cl)[0]);
        else this->vecs.push_back(cl);
    }
}
