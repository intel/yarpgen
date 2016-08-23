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

#ifndef IFCE_H
#define IFCE_H

#include <crosschain/typedefs.h>
#include <crosschain/stmt.h>
#include <crosschain/types.h>

namespace crosschain {
class Ifce : public Metadata {
protected:
    std::vector<std::shared_ptr<VecElem>> vecs;

public:
    Ifce (Metadata::NodeID id_) : Metadata(id_) {}

    virtual ull size () {
        return this->vecs.size();
    }

    virtual std::vector<std::shared_ptr<VecElem>> getData() {
        return this->vecs;
    }
};


// Iterable Ifce
class IfceIt : public Ifce {
protected:
    ull tripcount;
    std::shared_ptr<IterDeclStmt> it;

public:
    IfceIt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<Vector> v, ull A, ull ovp);
    IfceIt (std::shared_ptr<rl::Context> ctx, std::shared_ptr<Vector> v, std::shared_ptr<IterDeclStmt> it_, ull tc_);

    virtual std::shared_ptr<IterDeclStmt> getArrayIter () {return this->it;}
    virtual ull getTripCount () {return this->tripcount;}
};


// Partition Ifce
class IfcePart : public Ifce {
public:
    IfcePart (std::shared_ptr<rl::Context> ctx, std::shared_ptr<Vector> v, ull A, ull ovp);
};
}

#endif
