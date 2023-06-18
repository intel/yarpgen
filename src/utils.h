/*
Copyright (c) 2015-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

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

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>

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

// This class links together id (for example, type of unary operator) and its
// probability. Usually it is used in the form of std::vector<Probability<id>>
// and defines all possible variants for random decision (probability itself
// measured in parts, similarly to std::discrete_distribution). Preferably, sum
// of all probabilities in vector should be 100 (so we can treat 1 part as 1
// percent).
template <typename T> class Probability {
  public:
    Probability(T _id, uint64_t _prob) : id(_id), prob(_prob) {}
    T getId() { return id; }
    uint64_t getProb() { return prob; }

    void increaseProb(uint64_t add_prob) { prob += add_prob; }
    void zeroProb() { prob = 0; }
    void setProb(uint64_t _prob) { prob = _prob; }

    template <class U>
    friend std::ostream &operator<<(std::ostream &os,
                                    const Probability<U> &_prob);

  private:
    T id;
    uint64_t prob;
};

template <class T>
typename std::enable_if<!std::is_enum<T>::value, std::ostream &>::type
operator<<(std::ostream &os, const Probability<T> &_prob) {
    os << _prob.id << " : " << _prob.prob;
    return os;
}

template <class T>
typename std::enable_if<std::is_enum<T>::value, std::ostream &>::type
operator<<(std::ostream &os, const Probability<T> &_prob) {
    os << static_cast<long long int>(_prob.id) << " : " << _prob.prob;
    return os;
}

// According to the agreement, Random Value Generator is the only way to get any
// random value in YARPGen. It is used for different random decisions all over
// the source code.
class RandValGen {
  public:
    // Specific seed can be passed to constructor to reproduce the test.
    // Zero value is reserved (it notifies RandValGen that it can choose any)
    explicit RandValGen(uint64_t _seed);

    template <typename T> T getRandValue(T from, T to) {
        assert(from <= to && "Invalid range for random value generation");
        // Using long long instead of T is a hack.
        // getRandValue is used with all kind of integer types, including chars.
        // While standard is not allowing it to be used with
        // uniform_int_distribution<> algorithm. Though, clang and gcc ok with
        // it, but VS doesn't compile such code. For details see C++17,
        // $26.5.1.1e [rand.req.genl]. This issue is also discussed in issue
        // 2326 (closed as not a defect and reopened as feature request N4296).
        std::uniform_int_distribution<long long> dis(from, to);
        return static_cast<T>(dis(rand_gen));
    }

    template <typename T> T getRandValue() {
        // See note above about long long hack
        std::uniform_int_distribution<long long> dis(
            static_cast<long long>(std::numeric_limits<T>::min()),
            static_cast<long long>(std::numeric_limits<T>::max()));
        return static_cast<T>(dis(rand_gen));
    }

    template <typename T> T getRandUnsignedValue() {
        // See note above about long long hack
        std::uniform_int_distribution<unsigned long long> dis(
            0, static_cast<unsigned long long>(std::numeric_limits<T>::max()));
        return static_cast<T>(dis(rand_gen));
    }

    IRValue getRandValue(IntTypeID type_id);

    // Randomly chooses one of IDs, basing on std::vector<Probability<id>>.
    template <typename T> T getRandId(std::vector<Probability<T>> vec) {
        std::vector<double> discrete_dis_init;
        for (auto i : vec)
            discrete_dis_init.push_back(static_cast<double>(i.getProb()));

        std::discrete_distribution<size_t> discrete_dis(
            discrete_dis_init.begin(), discrete_dis_init.end());
        size_t idx = discrete_dis(rand_gen);
        return vec.at(idx).getId();
    }

    // Randomly choose element from a vector
    template <typename T> T &getRandElem(std::vector<T> &vec) {
        std::uniform_int_distribution<size_t> distr(0, vec.size() - 1);
        size_t idx = distr(rand_gen);
        return vec.at(idx);
    }

    // Randomly choose elements without replacement from a vector in order
    template <typename T>
    std::vector<T> getRandElemsInOrder(const std::vector<T> &vec, size_t num) {
        std::vector<T> ret;
        ret.reserve(num);
        std::sample(vec.begin(), vec.end(), std::back_inserter(ret), num,
                    rand_gen);
        return ret;
    }

    // Randomly choose elements without replacement from a vector
    template <typename T>
    std::vector<T> getRandElems(const std::vector<T> &vec, size_t num) {
        auto ret = getRandElemsInOrder(vec, num);
        std::shuffle(ret.begin(), ret.end(), rand_gen);
        return ret;
    }

    template <class T> void shuffleVector(std::vector<T> vec) {
        std::shuffle(vec.begin(), vec.end(), rand_gen);
    }

    // To improve variety of generated tests, we implement shuffling of
    // input probabilities (they are stored in GenPolicy).
    // TODO: sometimes this action increases test complexity, and tests becomes
    // non-generatable.
    template <typename T>
    void shuffleProb(std::vector<Probability<T>> &prob_vec) {
        uint64_t total_prob = 0;
        std::vector<double> discrete_dis_init;
        std::vector<Probability<T>> new_prob;
        for (auto i : prob_vec) {
            total_prob += i.getProb();
            discrete_dis_init.push_back(static_cast<double>(i.getProb()));
            new_prob.push_back(Probability<T>(i.getId(), 0));
        }

        std::uniform_int_distribution<uint64_t> dis(1ULL, total_prob);
        auto delta = static_cast<uint64_t>(
            round(((double)total_prob) / static_cast<double>(dis(rand_gen))));

        std::discrete_distribution<uint64_t> discrete_dis(
            discrete_dis_init.begin(), discrete_dis_init.end());
        for (uint64_t i = 0; i < total_prob; i += delta)
            new_prob.at(static_cast<size_t>(discrete_dis(rand_gen)))
                .increaseProb(delta);

        prob_vec = new_prob;
    }

    uint64_t getSeed() const { return seed; }
    void setSeed(uint64_t new_seed);
    void switchMutationStates();
    void setMutationSeed(uint64_t mutation_seed);

  private:
    uint64_t seed;
    std::mt19937_64 rand_gen;
    // Auxiliary random generator, used for mutation
    std::mt19937_64 prev_gen;
};

template <> inline bool RandValGen::getRandValue<bool>(bool from, bool to) {
    std::uniform_int_distribution<int> dis((int)from, (int)to);
    return (bool)dis(rand_gen);
}

extern std::shared_ptr<RandValGen> rand_val_gen;

class NameHandler {
  public:
    static NameHandler &getInstance() {
        static NameHandler instance;
        return instance;
    }
    NameHandler(const NameHandler &root) = delete;
    NameHandler &operator=(const NameHandler &) = delete;

    std::string getStubStmtIdx() { return std::to_string(stub_stmt_idx++); }
    std::string getVarName() { return "var_" + std::to_string(var_idx++); }
    std::string getArrayName() { return "arr_" + std::to_string(arr_idx++); }
    std::string getIterName() { return "i_" + std::to_string(iter_idx++); }

  private:
    NameHandler() : var_idx(0), arr_idx(0), iter_idx(0), stub_stmt_idx(0) {}

    uint32_t var_idx;
    uint32_t arr_idx;
    uint32_t iter_idx;
    uint32_t stub_stmt_idx;
};
} // namespace yarpgen
