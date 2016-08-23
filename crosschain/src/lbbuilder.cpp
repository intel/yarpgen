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

#include <crosschain/lbbuilder.h>

using namespace crosschain;


std::shared_ptr<IterDeclStmt> LBBuilder::genCoreIter (std::shared_ptr<Vector> v, ull A, ull level) {
    std::shared_ptr<IfceIt> ifce = std::make_shared<IfceIt>(this->context, v, A, 0);
    std::shared_ptr<IterDeclStmt> core_it = ifce->getArrayIter();
    this->tripcount = ifce->getTripCount();
    this->idecls.push_back(core_it);
    return core_it;
}


void LBBuilder::add_me_some_iscls(std::shared_ptr<Vector> v_in) {
    if(v_in->size() == 0) return;
    ull dicesize = 20;
    if (((*v_in)[0])->is_scalar())
        for(ull i = 0; i < v_in->size(); ++i)
            if(rl::rand_val_gen->get_rand_value<ull>(0, dicesize) == 0)
                this->iscls.push_back(std::static_pointer_cast<Cell>((*v_in)[i])->getData());
}


void LBBuilder::add_me_some_oscls(std::shared_ptr<Vector> v_in) {
    if(v_in->size() == 0) return;
    ull dicesize = 20;
    if (((*v_in)[0])->is_scalar())
        for(ull i = 0; i < v_in->size(); ++i) {
            if(rl::rand_val_gen->get_rand_value<ull>(0, dicesize) == 0)
                this->oscls.push_back(std::static_pointer_cast<Cell>((*v_in)[i])->getData());
        }
}


LBBuilder::LBBuilder (std::shared_ptr<rl::Context> ctx, std::vector<std::shared_ptr<Vector>> input) {
    this->id = rl::Node::NodeID::LBB;
    this->context = ctx;
    this->tripcount = 0;
    this->parent = NULL;
    this->iscls = ctx->get_extern_inp_sym_table()->get_variables();
    this->oscls = ctx->get_extern_out_sym_table()->get_variables();
    this->complexity_multiplier = 1;
    this->generate(input);
}


LBBuilder::LBBuilder (LBBuilder *p, std::vector<std::shared_ptr<Vector>> input) {
    assert(p != NULL);
    this->parent = p;
    this->id = rl::Node::NodeID::LBB;
    this->context = std::make_shared<rl::Context>(*(p->context->get_self_gen_policy()), std::shared_ptr<rl::Stmt>(), p->context);
    this->context->inc_loop_depth();
    this->context->setScopeId(ScopeIdGen::getNewID()); // Set a unique id for our new scope
    this->tripcount = 0;
    this->complexity_multiplier = this->parent->get_complexity_multiplier();
    this->iscls = p->iscls;
    this->generate(input);
}


std::vector<std::shared_ptr<Vector>> LBBuilder::cherryPick(std::vector<std::shared_ptr<Vector>> &vecs, VecElem::Purpose p_) {
    std::vector<std::shared_ptr<Vector>> ret;
    if (vecs.size() == 0) return ret;
    ull dicesize = vecs.size();

    if (p_ != VecElem::Purpose::NONE) {
        bool inp_empty = true;
        for (auto v : vecs) {
            if (v->getPurpose() == p_) {
                inp_empty = false;
                break;
            }
        }
        if (inp_empty) return ret;
    }

    while (ret.size() == 0)
        for (ull i = 0; i < vecs.size(); ++i)
            if(rl::rand_val_gen->get_rand_value<ull>(0, dicesize) == 0) {
                if ((p_ == VecElem::Purpose::NONE) || (vecs[i]->getPurpose() == p_)) {
                    ret.push_back(vecs[i]);
                    vecs.erase(vecs.begin()+i);
                    i --;
                }
            }

    return ret;
}


std::vector<std::shared_ptr<rl::Variable>> LBBuilder::cherryPick(std::vector<std::shared_ptr<rl::Variable>> &vars) {
    std::vector<std::shared_ptr<rl::Variable>> ret;
    if (vars.size() == 0) return ret;

    ull maxnum = 2;
    ull cnt = rl::rand_val_gen->get_rand_value<ull>(1, std::min((ull)vars.size(), maxnum));

    for (ull i = 0; i < cnt; ++i) {
        ull id = rl::rand_val_gen->get_rand_value<ull>(0, vars.size() - 1);
        ret.push_back(vars[id]);
        vars.erase(vars.begin()+id);
    }

    return ret;
}


void LBBuilder::generate (std::vector<std::shared_ptr<Vector>> input) {
    assert(input.size() > 0);

    {
    std::stringstream ss;
    ss << "Entering new loop (depth: " << this->context->get_loop_depth()
       << "/complexity up to this point: " << this->context->get_complexity() << "):";
    this->add(std::make_shared<CommentStmt>(this->context, ss.str(), 1));
    }

    for (auto v : input) this->add(std::make_shared<CommentStmt>(this->context, "input for loop: " + v->info()));

    // Do the header and main ind. variable (a.k.a. for(int i = k, i < k + 10; i += 5). We choose one of the vectors
    // to align smoothly on the ind. variable. Hope 'k' in the example is a loop invariant, or we are f*cked
    // Here we choose the size of 'A' (the cluster) and cluster overlap.

    std::shared_ptr<Vector> core_vec;
    if (this->context->get_loop_depth() < 4) {
        int cvid = rl::rand_val_gen->get_rand_value<ull>(0, input.size() - 1);
        core_vec = input[cvid];
    }
    else { // Try strategy which will not produce too much nested loops
        std::vector<std::shared_ptr<Vector>> oinput;
        for (auto v : input) if (v->getPurpose() != VecElem::Purpose::RONLY) oinput.push_back(v);
        assert(oinput.size() != 0);
        int cvid = rl::rand_val_gen->get_rand_value<ull>(0, oinput.size() - 1);
        core_vec = oinput[cvid];
    }

    int step = rl::rand_val_gen->get_rand_value<ull>(1, 1 + std::min(core_vec->size()/4, (ull)100));

    std::shared_ptr<IterDeclStmt> core_it = this->genCoreIter(core_vec, step, 0); // this sets up a tripcount
    std::shared_ptr<HeaderStmt> hdr = std::make_shared<HeaderStmt>(this->context, core_it, this->tripcount);
    this->add(hdr); // Add header to the tree
    // So here we got our header, we now have to work with whatever tripcount we got
    this->complexity_multiplier *= this->tripcount; // Track overall code execution complexity

    std::vector<std::shared_ptr<Vector>> input_prepared;
    // Cut some input vectors which are too long
    for (auto v : input) {
        if ((input.size() <= 20) &&
            (v->getPurpose() == VecElem::Purpose::RONLY) && !(v->equals(core_vec)) && 
            (v->size() >= this->tripcount * 2) &&
            (rl::rand_val_gen->get_rand_value<ull>(0, 3) == 0)) {

            ull ovp = rl::rand_val_gen->get_rand_value<ull>(0, this->tripcount/5);

            std::shared_ptr<IfcePart> ifce = std::make_shared<IfcePart>(this->context, v, this->tripcount, ovp);
            for (auto v_ : ifce->getData()) {
                if (v_->is_scalar()) this->add(v_);
                if (v_->is_vector()) input_prepared.push_back(std::static_pointer_cast<Vector>(v_));
            }
            continue;
        }

        input_prepared.push_back(v);
    }



    // First, cut (partition) all input/output vecs we have according to tripcount
    // The ones we were able to cut are fine, the ones we didn't, go with tmp variables for the next step
    for (auto v : input_prepared) {
        std::shared_ptr<IfceIt> ifce = std::make_shared<IfceIt>(this->context, v, core_it, this->tripcount);
        this->add(ifce->getData()); // Each vector individually remembers what needs to be done with it, Ifce will have
        // some vectors that do not need to be initialized and do not align with our iterator deleted, while add will
        // sort the data to our i/o-scls and i/o-vecs fields
    }

    for (auto v : this->ivecs) hdr->add(std::make_shared<CommentStmt>(this->context, "current ivecs: " + v->info()));
    for (auto v : this->ovecs) hdr->add(std::make_shared<CommentStmt>(this->context, "current ovecs: " + v->info()));

    // TODO: Do some cool stuff with IfceRnd to address 'uncuttable' vectors
    //for (auto input_vec : this->ivecs_fail) {
    //    std::shared_ptr<IfceRnd> ifce = std::make_shared<IfceRnd>(input_vec, this->context);
    //    ...

    // If vectors are small, access em as they are
    //for (auto v : this->ivecs) this->add_me_some_iscls(v);
    //for (auto v : this->ovecs) this->add_me_some_oscls(v);
    // Now the main part! Cut some vectors -> make some tmp's -> make a new body -> do some math -> repeat
    // You never know what you will need. Cherry pick parts from 'fail' vector list


    // Consider loop header execution cost as '1'
    this->context->add_to_complexity(this->get_complexity_multiplier() * 1);

    while ((this->oscls.size() > 0) || (this->ovecs.size() > 0)) {
        // Set up probabilities
        enum Parts {DO_TMP, DEEPER, SCALAR};
        std::vector<rl::Probability<Parts>> decisions;
        decisions.push_back(rl::Probability<Parts>(Parts::DO_TMP, ((this->ivecs.size() == 0) || (this->context->get_complexity() > 500000)) ? 0 : 20));
        decisions.push_back(rl::Probability<Parts>(Parts::DEEPER, (this->ovecs.size() == 0) ? 0 : 30));
        decisions.push_back(rl::Probability<Parts>(Parts::SCALAR, (this->oscls.size() == 0) ? 0 : 50));
        Parts decision = rl::rand_val_gen->get_rand_id(decisions);

        // 1) Cut some vectors (TODO)

        // 2) Make some tmp's
        if ((this->ivecs.size() != 0) && (this->context->get_complexity() <= 500000) && (decision == Parts::DO_TMP)) {
            std::shared_ptr<VectorDeclStmt> new_vec = std::make_shared<VectorDeclStmt>(this->context);
            std::shared_ptr<Vector> tmp_v = new_vec->get_data();
            hdr->add(new_vec);

            // Create a new tmp variable
            // Interesting thing about local constants is that they are definitely read-write.
            // First, you need to initialize it, and only then use it
            if ((tmp_v ->getKind() == VecElem::Kind::C_ARR) || (tmp_v ->getKind() == VecElem::Kind::STD_ARR)) {
                std::vector<std::shared_ptr<Vector>> candidates = this->cherryPick(this->ovecs);
                for(auto v : this->ivecs) candidates.push_back(v);
                candidates.push_back(tmp_v);
                hdr->add(std::make_shared<LBBuilder>(this, candidates)); // now 'initialize' tmp
            }
            else {
                hdr->add(new_vec->getInitStmt());
            }

            // Use the tmp in the ramainder of the scope
            tmp_v->setPurpose(VecElem::Purpose::RONLY);
            this->add(tmp_v);
            this->add_me_some_iscls(tmp_v);
        }

        // 3) Make a new body
        if ((this->ovecs.size() != 0) && (decision == Parts::DEEPER)) {
            std::vector<std::shared_ptr<Vector>> candidates;
            std::vector<std::shared_ptr<Vector>> to_return; // These are initialized TO_INIT's, need to put them to ivecs
            if (this->ivecs.size() == 0)
                candidates = this->cherryPick(this->ovecs, VecElem::Purpose::TO_INIT);
            if (candidates.size() == 0)
                candidates = this->cherryPick(this->ovecs);
            for (auto v : candidates)
                if (v->getPurpose() == VecElem::Purpose::TO_INIT)
                    to_return.push_back(v);

            // Make a new loop
            for(auto v : this->ivecs) candidates.push_back(v);
            hdr->add(std::make_shared<LBBuilder>(this, candidates));

            // Put initializet vecs to ivecs
            for (auto v : to_return) {
                assert(v->getPurpose() == VecElem::Purpose::RONLY);
                this->ivecs.push_back(v);
            }

            for (auto v : this->ivecs) hdr->add(std::make_shared<CommentStmt>(this->context, "current ivecs: " + v->info()));
            for (auto v : this->ovecs) hdr->add(std::make_shared<CommentStmt>(this->context, "current ovecs: " + v->info()));
        }

        // 4) Do some math
        if ((this->oscls.size() != 0) && (decision == Parts::SCALAR)) {
            std::vector<std::shared_ptr<rl::Variable>> o_candidates = this->cherryPick(this->oscls);
            for(auto oscl : o_candidates) {
                std::shared_ptr<RInitStmtNVC> init_var_stmt = std::make_shared<RInitStmtNVC>(this->context, this->iscls, oscl);
                hdr->add(std::make_shared<CommentStmt>(this->context, "next arithm: " + init_var_stmt->info()));
                hdr->add(init_var_stmt);
                // The cost of the instruction is its actual cost times the number it is going to be executed
                this->context->add_to_complexity(this->get_complexity_multiplier() * 1);
            }
        }
    }

    for (auto v : input)
        if (v->getPurpose() == VecElem::Purpose::TO_INIT)
            v->setPurpose(VecElem::Purpose::RONLY);
}


std::string LBBuilder::emit (std::string offset) {
    std::stringstream ss;
    for (auto e : this->module) {
        if ((e->get_id() == rl::Node::NodeID::CMNT) && (e->is_dead()))
            continue;

        ss << e->emit(offset) << "\n";
    }

    return ss.str();
}
