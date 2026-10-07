/*
Copyright (c) 2019-2020, Intel Corporation
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

#include "context.h"
#include "data.h"
#include "expr.h"

#include <iostream>

using namespace yarpgen;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__            \
                      << ", function " << __func__ << "():\n    " << (msg)     \
                      << std::endl;                                            \
            abort();                                                           \
        }                                                                      \
    } while (false)

// Evaluates "acc <op>= inc" over "iters" iterations and reports whether it was
// found to have UB.
static bool reductionHasUB(IntTypeID type_id, IRValue::AbsValue acc_val,
                           IRValue::AbsValue inc_val, BinaryOp bin_op,
                           int64_t iters, bool is_omp_reduction) {
    auto type = IntegralType::init(type_id);
    auto acc =
        std::make_shared<ScalarVar>("acc", type, IRValue(type_id, acc_val));
    auto inc =
        std::make_shared<ScalarVar>("inc", type, IRValue(type_id, inc_val));
    auto assign = std::make_shared<AssignmentExpr>(ScalarVarUseExpr::init(acc),
                                                   ScalarVarUseExpr::init(inc));
    auto reduction = std::make_shared<ReductionExpr>(
        assign, bin_op, LibCallKind::MAX_LIB_CALL_KIND,
        /*is_degenerate*/ false, is_omp_reduction);

    EvalCtx ctx;
    ctx.total_iter_num = iters;
    return reduction->evaluate(ctx)->hasUB();
}

static void ompReductionTest() {
    // Serially "x += inc" stays in range, but the identity-seeded partial
    // 0 + 2 * inc is below INT_MIN.
    CHECK(!reductionHasUB(IntTypeID::INT, {false, 1886950276},
                          {true, 1640729410}, BinaryOp::ADD, 2, false),
          "signed add, serial");
    CHECK(reductionHasUB(IntTypeID::INT, {false, 1886950276},
                         {true, 1640729410}, BinaryOp::ADD, 2, true),
          "signed add, omp partials");

    // unsigned short is promoted to int: serially every product is 0, but a
    // lane seeded with the identity 1 computes 65535 * 65535.
    CHECK(!reductionHasUB(IntTypeID::USHORT, {false, 0}, {false, 65535},
                          BinaryOp::MUL, 2, false),
          "unsigned short mul, serial");
    CHECK(reductionHasUB(IntTypeID::USHORT, {false, 0}, {false, 65535},
                         BinaryOp::MUL, 2, true),
          "unsigned short mul, omp lane step");
    // No single lane step overflows (3 * 65535 fits), but merging two lanes
    // of 16 iterations each does: 3^16 mod 2^16 = 55105, squared.
    CHECK(reductionHasUB(IntTypeID::USHORT, {false, 0}, {false, 3},
                         BinaryOp::MUL, 32, true),
          "unsigned short mul, omp lane merge");
    CHECK(!reductionHasUB(IntTypeID::USHORT, {false, 0}, {false, 1},
                          BinaryOp::MUL, 32, true),
          "unsigned short mul by 1, omp");
    // 255 * 255 always fits in int
    CHECK(!reductionHasUB(IntTypeID::UCHAR, {false, 0}, {false, 255},
                          BinaryOp::MUL, 32, true),
          "unsigned char mul, omp");
    // unsigned int is not promoted to a signed type, so it just wraps
    CHECK(!reductionHasUB(IntTypeID::UINT, {false, 0}, {false, 4294967295},
                          BinaryOp::MUL, 32, true),
          "unsigned int mul, omp");
}

int main() {
    ompReductionTest();

    IRValue start_val(IntTypeID::INT);
    start_val.setValue({false, 0});
    auto start_expr = std::make_shared<ConstantExpr>(start_val);

    IRValue end_val(IntTypeID::INT);
    end_val.setValue({false, 0});
    auto end_expr = std::make_shared<ConstantExpr>(end_val);
}
