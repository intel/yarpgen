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

#pragma once

#include "enums.h"

#include <iostream>
#include <memory>
#include <random>

namespace yarpgen {

class IRValue;

// Macros for error handling
#define ERROR(err_message)                                                     \
    do {                                                                       \
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__                \
                  << ", function " << __func__ << "():\n    " << (err_message) \
                  << std::endl;                                                \
        abort();                                                               \
    } while (false)

// According to the agreement, Random Value Generator is the only way to get any
// random value in YARPGen. It is used for different random decisions all over
// the source code.
class RandValGen {
  public:
    // Specific seed can be passed to constructor to reproduce the test.
    // Zero value is reserved (it notifies RandValGen that it can choose any)
    RandValGen(uint64_t _seed);

    template <typename T> T getRandValue(T from, T to) {
        // Using long long instead of T is a hack.
        // getRandValue is used with all kind of integer types, including chars.
        // While standard is not allowing it to be used with
        // uniform_int_distribution<> algorithm. Though, clang and gcc ok with
        // it, but VS doesn't compile such code. For details see C++17,
        // $26.5.1.1e [rand.req.genl]. This issue is also discussed in issue
        // 2326 (closed as not a defect and reopened as feature request N4296).
        std::uniform_int_distribution<long long> dis(from, to);
        return dis(rand_gen);
    }

    template <typename T> T getRandValue() {
        // See note above about long long hack
        std::uniform_int_distribution<long long> dis(
            static_cast<long long>(std::numeric_limits<T>::min()),
            static_cast<long long>(std::numeric_limits<T>::max()));
        return dis(rand_gen);
    }

    IRValue getRandValue(IntTypeID type_id);

  private:
    uint64_t seed;
    std::mt19937_64 rand_gen;
};

template <> inline bool RandValGen::getRandValue<bool>(bool from, bool to) {
    std::uniform_int_distribution<int> dis((int)from, (int)to);
    return (bool)dis(rand_gen);
}

extern std::shared_ptr<RandValGen> rand_val_gen;
} // namespace yarpgen