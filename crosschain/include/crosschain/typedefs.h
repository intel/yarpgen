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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <cassert>
#include <memory>
#include <map>
#include <algorithm>


namespace crosschain {
typedef long long int sll;
typedef unsigned long long int ull;



class Metadata {
public:
    std::string name;
    enum NodeID { NONE, IFCEIT };

protected:
    NodeID id;

public:
    Metadata () : id(NONE) {
        this->name = this->Ty_to_str ();
    }

    Metadata (NodeID id_) : id(id_) {
        this->name = this->Ty_to_str ();
    }

    virtual std::string Ty_to_str () {
        switch (this->id) {
            case NONE:   return "None specified";
            case IFCEIT: return "Iterable Ifce";
            default:     return "-unsupported-";
        }
    }
};
}


#endif
