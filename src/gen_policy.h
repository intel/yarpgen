/*
Copyright (c) 2015-2020, Intel Corporation
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

#include "utils.h"
#include <cstddef>
#include <vector>

namespace yarpgen {

class GenPolicy {
  public:
    GenPolicy();

    // Maximal number of loops in a single LoopSequence
    size_t loop_seq_num_lim;
    // Distribution of loop numbers for a LoopSequence
    std::vector<Probability<size_t>> loop_seq_num_distr;

    // Maximal depth of a single LoopNest
    size_t loop_nest_depth_lim;
    // Distribution of depths for a LoopNest
    std::vector<Probability<size_t>> loop_nest_depth_distr;

    // Hard threshold for loop depth
    size_t loop_depth_limit;

    // Number of statements in a scope
    size_t scope_stmt_min_num;
    size_t scope_stmt_max_num;
    std::vector<Probability<size_t>> scope_stmt_num_distr;

    // Number of iterators per loop
    size_t min_iters_num;
    size_t max_iters_num;
    std::vector<Probability<size_t>> iters_num_distr;

    // TODO: we want to replace constant parameters of iterators with something
    // smarter

    // End limits for iterators
    size_t iters_end_limit_min;
    size_t iter_end_limit_max;
    // Step distribution for iterators
    std::vector<Probability<size_t>> iters_step_distr;

    // Distribution of statements type for structure generation
    std::vector<Probability<IRNodeKind>> stmt_kind_struct_distr;

    // Distribution of statements type for population generation
    std::vector<Probability<IRNodeKind>> stmt_kind_pop_distr;

    // Distribution of available integral types
    std::vector<Probability<IntTypeID>> int_type_distr;

    // Number of external input variables
    size_t min_inp_vars_num;
    size_t max_inp_vars_num;

    // Number of new arrays that we create in each loop scope
    size_t min_new_arr_num;
    size_t max_new_arr_num;
    std::vector<Probability<size_t>> new_arr_num_distr;

    // Output kind probability
    std::vector<Probability<DataKind>> out_kind_distr;

    // Maximal depth of arithmetic expression
    size_t max_arith_depth;
    // Distribution of nodes in arithmetic expression
    std::vector<Probability<IRNodeKind>> arith_node_distr;
    // Unary operator distribution
    std::vector<Probability<UnaryOp>> unary_op_distr;
    // Binary operator distribution
    std::vector<Probability<BinaryOp>> binary_op_distr;

    static size_t leaves_prob_bump;

  private:
    template <typename T>
    void uniformProbFromMax(std::vector<Probability<T>> &distr, size_t max_num,
                            size_t min_num = 0);
};

} // namespace yarpgen