/*
Copyright (c) 2019, Intel Corporation

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

//////////////////////////////////////////////////////////////////////////////

#include "utils.h"
#include <memory>

using namespace yarpgen;

std::shared_ptr<RandValGen> yarpgen::rand_val_gen;

RandValGen::RandValGen (uint64_t _seed) {
    if (_seed != 0) {
        seed = _seed;
    }
    else {
        std::random_device rd;
        seed = rd ();
    }
    std::cout << "/*SEED " << seed << "*/" << std::endl;
    rand_gen = std::mt19937_64(seed);
}