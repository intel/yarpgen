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

#include <random>

#include "enums.h"
#include "ir_value.h"
#include "type.h"

using namespace yarpgen;

void type_test() {
    // Check for int type initialization
    for (auto i = static_cast<int>(IntTypeID::BOOL);
         i < static_cast<int>(IntTypeID::MAX_INT_TYPE_ID); ++i)
        for (auto j = static_cast<int>(CVQualifier::NONE);
             j <= static_cast<int>(CVQualifier::CONST_VOLAT); ++j)
            for (int k = false; k <= true; ++k) {
                std::shared_ptr<IntegralType> ptr_to_type = IntegralType::init(
                    static_cast<IntTypeID>(i), static_cast<bool>(k),
                    static_cast<CVQualifier>(j));
            }
    for (auto i = static_cast<int>(IntTypeID::BOOL);
         i < static_cast<int>(IntTypeID::MAX_INT_TYPE_ID); ++i)
        for (auto j = static_cast<int>(CVQualifier::NONE);
             j <= static_cast<int>(CVQualifier::CONST_VOLAT); ++j)
            for (int k = false; k <= true; ++k) {
                std::shared_ptr<IntegralType> ptr_to_type = IntegralType::init(
                    static_cast<IntTypeID>(i), static_cast<bool>(k),
                    static_cast<CVQualifier>(j));
                ptr_to_type->dbgDump();
                std::cout << "-------------------" << std::endl;
            }

    // Check for array type initialization
    for (auto i = static_cast<int>(IntTypeID::BOOL);
         i < static_cast<int>(IntTypeID::MAX_INT_TYPE_ID); ++i)
        for (auto j = static_cast<int>(CVQualifier::NONE);
             j <= static_cast<int>(CVQualifier::CONST_VOLAT); ++j)
            for (int k = false; k <= true; ++k) {
                std::shared_ptr<IntegralType> ptr_to_int_type =
                    IntegralType::init(static_cast<IntTypeID>(i),
                                       static_cast<bool>(k),
                                       static_cast<CVQualifier>(j));
                std::vector<size_t> dims = {static_cast<size_t>(i),
                                            static_cast<size_t>(j),
                                            static_cast<size_t>(k)};
                std::shared_ptr<ArrayType> ptr_to_type =
                    ArrayType::init(ptr_to_int_type, dims, static_cast<bool>(k),
                                    static_cast<CVQualifier>(j));
            }

    for (auto i = static_cast<int>(IntTypeID::BOOL);
         i < static_cast<int>(IntTypeID::MAX_INT_TYPE_ID); ++i)
        for (auto j = static_cast<int>(CVQualifier::NONE);
             j <= static_cast<int>(CVQualifier::CONST_VOLAT); ++j)
            for (int k = false; k <= true; ++k) {
                std::shared_ptr<IntegralType> ptr_to_int_type =
                    IntegralType::init(static_cast<IntTypeID>(i),
                                       static_cast<bool>(k),
                                       static_cast<CVQualifier>(j));
                std::vector<size_t> dims = {static_cast<size_t>(i),
                                            static_cast<size_t>(j),
                                            static_cast<size_t>(k)};
                std::shared_ptr<ArrayType> ptr_to_type =
                    ArrayType::init(ptr_to_int_type, dims, static_cast<bool>(k),
                                    static_cast<CVQualifier>(j));

                ptr_to_type->dbgDump();
                std::cout << "-------------------" << std::endl;
            }
}

//////////////////////////////////////////////////////////////////////////////

// Will be used to obtain a seed for the random number engine
static std::random_device rd;
static std::mt19937 generator;

//////////////////////////////////////////////////////////////////////////////
// Utility functions to perform check of the result

template <typename T>
void checkBinaryNoUB(IRValue &ret, IRValue &a, IRValue &b,
                     std::function<T(T &, T &)> foo,
                     const std::string &test_name) {
    if (ret.getUBCode() != UBKind::NoUB ||
        ret.getValueRef<T>() != foo(a.getValueRef<T>(), b.getValueRef<T>())) {
        std::cout << "ERROR: " << test_name << " " << typeid(T).name();
        std::cout << " ERROR NoUB: " << a.getValueRef<T>() << " | "
                  << b.getValueRef<T>() << std::endl;
    }
}

template <typename T>
void checkBinaryForUB(IRValue &ret, IRValue &a, IRValue &b,
                      const std::string &test_name) {
    if (ret.getUBCode() == UBKind::NoUB ||
        ret.getValueRef<T>() != static_cast<T>(0)) {
        std::cout << "ERROR: " << test_name << " " << typeid(T).name();
        std::cout << " ERROR UB: " << a.getValueRef<T>() << " | "
                  << b.getValueRef<T>() << std::endl;
    }
}

template <typename T>
void checkUnaryNoUB(IRValue &ret, IRValue &a, std::function<T(T &)> foo,
                    const std::string &test_name) {
    if (ret.getUBCode() != UBKind::NoUB ||
        ret.getValueRef<T>() != foo(a.getValueRef<T>())) {
        std::cout << "ERROR: " << test_name << " " << typeid(T).name();
        std::cout << " ERROR NoUB: " << a.getValueRef<T>() << std::endl;
    }
}

template <typename T>
void checkUnaryForUB(IRValue &ret, IRValue &a, const std::string &test_name) {
    if (ret.getUBCode() == UBKind::NoUB ||
        ret.getValueRef<T>() != static_cast<T>(0)) {
        std::cout << "ERROR: " << test_name << " " << typeid(T).name();
        std::cout << " ERROR UB: " << a.getValueRef<T>() << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, void>::type
singleAddTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);
    auto distr = std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    a.getValueRef<T>() = distr(generator);
    a.setUBCode(UBKind::NoUB);
    b.getValueRef<T>() = distr(generator);
    b.setUBCode(UBKind::NoUB);
    IRValue ret = a + b;
    checkBinaryNoUB<T>(ret, a, b, std::plus<T>(), __FUNCTION__);
}

template <typename T>
typename std::enable_if<std::is_signed<T>::value, void>::type
singleAddTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);

    auto a_distr = std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    a.getValueRef<T>() = a_distr(generator);
    a.setUBCode(UBKind::NoUB);
    // Determine if we want to test UB
    auto bool_distr = std::uniform_int_distribution<int>(0, 1);
    auto test_ub = static_cast<bool>(bool_distr(generator));

    T b_min = 0;
    T b_max = 0;

    if (a.getValueRef<T>() > 0) {
        if (test_ub) {
            b_min = std::numeric_limits<T>::max() - a.getValueRef<T>() + 1;
            b_max = std::numeric_limits<T>::max();
        }
        else {
            b_min = std::numeric_limits<T>::min();
            b_max = std::numeric_limits<T>::max() - a.getValueRef<T>();
        }
    }
    else if (a.getValueRef<T>() < 0) {
        if (test_ub) {
            b_min = std::numeric_limits<T>::min();
            b_max = std::numeric_limits<T>::min() - a.getValueRef<T>() - 1;
        }
        else {
            b_min = std::numeric_limits<T>::min() - a.getValueRef<T>();
            b_max = std::numeric_limits<T>::max();
        }
    }
    else {
        b_min = std::numeric_limits<T>::min();
        b_max = std::numeric_limits<T>::max();
    }

    assert(b_min < b_max);
    auto b_distr = std::uniform_int_distribution<T>(b_min, b_max);
    b.getValueRef<T>() = b_distr(generator);
    b.setUBCode(UBKind::NoUB);

    IRValue ret = a + b;
    if (test_ub)
        checkBinaryForUB<T>(ret, a, b, __FUNCTION__);
    else
        checkBinaryNoUB<T>(ret, a, b, std::plus<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, void>::type
singleSubTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);
    auto distr = std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    a.getValueRef<T>() = distr(generator);
    a.setUBCode(UBKind::NoUB);
    b.getValueRef<T>() = distr(generator);
    b.setUBCode(UBKind::NoUB);
    IRValue ret = a - b;
    checkBinaryNoUB<T>(ret, a, b, std::minus<T>(), __FUNCTION__);
}

template <typename T>
typename std::enable_if<std::is_signed<T>::value, void>::type
singleSubTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);

    auto a_distr = std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    a.getValueRef<T>() = a_distr(generator);
    a.setUBCode(UBKind::NoUB);

    // Determine if we want to test UB
    auto bool_distr = std::uniform_int_distribution<int>(0, 1);
    auto test_ub = static_cast<bool>(bool_distr(generator));

    T b_min = 0;
    T b_max = 0;

    if (a.getValueRef<T>() > 0) {
        if (test_ub) {
            b_min = std::numeric_limits<T>::min();
            b_max = a.getValueRef<T>() - std::numeric_limits<T>::max() - 1;
        }
        else {
            b_min = a.getValueRef<T>() - std::numeric_limits<T>::max();
            b_max = std::numeric_limits<T>::max();
        }
    }
    else if (a.getValueRef<T>() < 0) {
        if (test_ub) {
            b_min = a.getValueRef<T>() - std::numeric_limits<T>::min() + 1;
            b_max = std::numeric_limits<T>::max();
        }
        else {
            b_min = std::numeric_limits<T>::min();
            b_max = a.getValueRef<T>() - std::numeric_limits<T>::min();
        }
    }
    else {
        b_min = std::numeric_limits<T>::min();
        b_max = std::numeric_limits<T>::max();
    }

    assert(b_min < b_max);
    auto b_distr = std::uniform_int_distribution<T>(b_min, b_max);
    b.getValueRef<T>() = b_distr(generator);
    b.setUBCode(UBKind::NoUB);

    IRValue ret = a - b;
    if (test_ub)
        checkBinaryForUB<T>(ret, a, b, __FUNCTION__);
    else
        checkBinaryNoUB<T>(ret, a, b, std::minus<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, void>::type
singleMulTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);
    auto distr = std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    a.getValueRef<T>() = distr(generator);
    a.setUBCode(UBKind::NoUB);
    b.getValueRef<T>() = distr(generator);
    b.setUBCode(UBKind::NoUB);
    IRValue ret = a * b;
    checkBinaryNoUB<T>(ret, a, b, std::multiplies<T>(), __FUNCTION__);
}

template <typename T>
typename std::enable_if<std::is_signed<T>::value, void>::type
singleMulTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);

    auto min_max_distr = std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    a.getValueRef<T>() = min_max_distr(generator);
    a.setUBCode(UBKind::NoUB);

    // Determine if we want to test UB
    auto bool_distr = std::uniform_int_distribution<int>(0, 1);
    auto test_ub = static_cast<bool>(bool_distr(generator));

    if (a.getValueRef<T>() == static_cast<T>(0)) {
        b.getValueRef<T>() = min_max_distr(generator);
        b.setUBCode(UBKind::NoUB);
        test_ub = false;
    }
    else if (a.getValueRef<T>() ==
             static_cast<T>(-1)) { // Check for special case
        if (test_ub) {
            b.getValueRef<T>() = std::numeric_limits<T>::min();
            b.setUBCode(UBKind::NoUB);
        }
        else {
            auto distr = std::uniform_int_distribution<T>(
                std::numeric_limits<T>::min() + 1,
                std::numeric_limits<T>::max());
            b.getValueRef<T>() = distr(generator);
            b.setUBCode(UBKind::NoUB);
        }
    }
    else {
        if (test_ub) {
            auto distr = min_max_distr;
            if (bool_distr(
                    generator)) { // If we want to check positive overflow
                if (a.getValueRef<T>() > 0)
                    distr = std::uniform_int_distribution<T>(
                        std::numeric_limits<T>::max() / a.getValueRef<T>(),
                        std::numeric_limits<T>::max());
                else
                    distr = std::uniform_int_distribution<T>(
                        std::numeric_limits<T>::min(),
                        std::numeric_limits<T>::max() / a.getValueRef<T>());
            }
            else {
                if (a.getValueRef<T>() > 0)
                    distr = std::uniform_int_distribution<T>(
                        std::numeric_limits<T>::min(),
                        std::numeric_limits<T>::max() / a.getValueRef<T>());
                else
                    distr = std::uniform_int_distribution<T>(
                        std::numeric_limits<T>::min() / a.getValueRef<T>(),
                        std::numeric_limits<T>::max());
            }
            b.getValueRef<T>() = distr(generator);
            b.setUBCode(UBKind::NoUB);
        }
        else {
            b.getValueRef<T>() = min_max_distr(generator) / a.getValueRef<T>();
            b.setUBCode(UBKind::NoUB);
        }
    }

    IRValue ret = a * b;
    if (test_ub)
        checkBinaryForUB<T>(ret, a, b, __FUNCTION__);
    else
        checkBinaryNoUB<T>(ret, a, b, std::multiplies<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T> void singleDivModTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);

    // Determine if we want to test UB
    auto bool_distr = std::uniform_int_distribution<int>(0, 1);
    auto test_ub = static_cast<bool>(bool_distr(generator));

    auto distr = std::uniform_int_distribution<T>(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

    if (test_ub) {
        // Test for special case of overflow
        if (std::is_signed<T>::value && bool_distr(generator)) {
            a.getValueRef<T>() = std::numeric_limits<T>::min();
            a.setUBCode(UBKind::NoUB);
            b.getValueRef<T>() = -1;
            b.setUBCode(UBKind::NoUB);
        }
        else {
            a.getValueRef<T>() = distr(generator);
            a.setUBCode(UBKind::NoUB);
            b.getValueRef<T>() = 0;
            b.setUBCode(UBKind::NoUB);
        }
    }
    else {
        a.getValueRef<T>() = distr(generator);
        a.setUBCode(UBKind::NoUB);
        b.getValueRef<T>() = distr(generator);
        b.setUBCode(UBKind::NoUB);
        if ((std::is_signed<T>::value &&
             a.getValueRef<T>() == std::numeric_limits<T>::min() &&
             b.getValueRef<T>() == static_cast<T>(-1)) ||
            b.getValueRef<T>() == 0)
            test_ub = true;
    }

    IRValue ret = a / b;
    if (test_ub)
        checkBinaryForUB<T>(ret, a, b, __FUNCTION__);
    else
        checkBinaryNoUB<T>(ret, a, b, std::divides<T>(), __FUNCTION__);

    ret = a % b;
    if (test_ub)
        checkBinaryForUB<T>(ret, a, b, __FUNCTION__);
    else
        checkBinaryNoUB<T>(ret, a, b, std::modulus<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////
// It is a messy kludge. MSVC doesn't allow to get uniform distribution for
// anything that is less than int.

template <typename T>
static typename std::enable_if<
    std::integral_constant<bool, sizeof(T) < sizeof(int32_t)>::value,
    std::uniform_int_distribution<int>>::type
getDistribution() {
    return std::uniform_int_distribution<int32_t>(
        static_cast<int32_t>(std::numeric_limits<T>::min()),
        static_cast<int32_t>(std::numeric_limits<T>::max()));
}

template <typename T>
static typename std::enable_if<
    std::integral_constant<bool, sizeof(T) >= sizeof(int32_t)>::value,
    std::uniform_int_distribution<T>>::type
getDistribution() {
    return std::uniform_int_distribution<T>(std::numeric_limits<T>::min(),
                                            std::numeric_limits<T>::max());
}

//////////////////////////////////////////////////////////////////////////////

template <typename T> void singleCmpTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);

    auto bool_distr = std::uniform_int_distribution<int>(0, 1);
    auto distr = getDistribution<T>();
    a.getValueRef<T>() = static_cast<T>(distr(generator));
    a.setUBCode(UBKind::NoUB);
    b.getValueRef<T>() = static_cast<T>(distr(generator));
    b.setUBCode(UBKind::NoUB);

    if (bool_distr(generator)) {
        b.getValueRef<T>() = a.getValueRef<T>();
        b.setUBCode(UBKind::NoUB);
    }

    IRValue ret = a < b;
    checkBinaryNoUB<T>(ret, a, b, std::less<T>(), __FUNCTION__);

    ret = a > b;
    checkBinaryNoUB<T>(ret, a, b, std::greater<T>(), __FUNCTION__);

    ret = a <= b;
    checkBinaryNoUB<T>(ret, a, b, std::less_equal<T>(), __FUNCTION__);

    ret = a >= b;
    checkBinaryNoUB<T>(ret, a, b, std::greater_equal<T>(), __FUNCTION__);

    ret = a == b;
    checkBinaryNoUB<T>(ret, a, b, std::equal_to<T>(), __FUNCTION__);

    ret = a != b;
    checkBinaryNoUB<T>(ret, a, b, std::not_equal_to<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

void singleLogicalAndOrTest() {
    IRValue a(IntTypeID::BOOL);
    IRValue b(IntTypeID::BOOL);

    auto bool_distr = std::uniform_int_distribution<int>(0, 1);
    a.getValueRef<bool>() = static_cast<bool>(bool_distr(generator));
    a.setUBCode(UBKind::NoUB);
    b.getValueRef<bool>() = static_cast<bool>(bool_distr(generator));
    b.setUBCode(UBKind::NoUB);

    IRValue ret = a && b;
    checkBinaryNoUB<bool>(ret, a, b, std::logical_and<>(), __FUNCTION__);

    ret = a || b;
    checkBinaryNoUB<bool>(ret, a, b, std::logical_or<>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T> void singleBitwiseAndOrXorTest(IntTypeID type_id) {
    IRValue a(type_id);
    IRValue b(type_id);

    auto distr = getDistribution<T>();
    a.getValueRef<T>() = static_cast<T>(distr(generator));
    a.setUBCode(UBKind::NoUB);
    b.getValueRef<T>() = static_cast<T>(distr(generator));
    b.setUBCode(UBKind::NoUB);

    IRValue ret = a & b;
    checkBinaryNoUB<T>(ret, a, b, std::bit_and<T>(), __FUNCTION__);

    ret = a | b;
    checkBinaryNoUB<T>(ret, a, b, std::bit_or<T>(), __FUNCTION__);

    ret = a ^ b;
    checkBinaryNoUB<T>(ret, a, b, std::bit_xor<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T> static inline size_t findMSB(T x) {
    // TODO: implementation-defined!
    if (std::is_signed<T>::value && x < 0)
        return sizeof(T) * CHAR_BIT;
    size_t ret = 0;
    while (x != 0) {
        ret++;
        x = x >> 1;
    }
    return ret;
}

template <typename LT, typename RT>
void singleLeftRightShiftTest(IntTypeID lhs_type_id, IntTypeID rhs_type_id) {
    IRValue a(lhs_type_id);
    IRValue b(rhs_type_id);

    auto a_distr = getDistribution<LT>();
    auto b_distr = getDistribution<RT>();

    auto check_func = [](IRValue &a, IRValue &b, IRValue &ret) {
        if (ret.getUBCode() == UBKind::NoUB ||
            ret.getValueRef<LT>() != static_cast<LT>(0)) {
            std::cout << "ERROR: " << __FUNCTION__ << typeid(LT).name();
            std::cout << "ERROR UB: " << a.getValueRef<LT>() << " | "
                      << b.getValueRef<RT>() << std::endl;
        }
    };

    // Rhs is too large (exceeds size of type)
    a.getValueRef<LT>() = a_distr(generator);
    a.setUBCode(UBKind::NoUB);
    b_distr = std::uniform_int_distribution<RT>(sizeof(LT) * CHAR_BIT,
                                                std::numeric_limits<RT>::max());
    b.getValueRef<RT>() = b_distr(generator);
    b.setUBCode(UBKind::NoUB);
    IRValue ret = a << b;
    check_func(a, b, ret);
    ret = a >> b;
    check_func(a, b, ret);

    // Rhs is negative
    if (std::is_signed<RT>::value) {
        a.getValueRef<LT>() = a_distr(generator);
        a.setUBCode(UBKind::NoUB);
        b_distr = std::uniform_int_distribution<RT>(
            std::numeric_limits<RT>::min(), -1);
        b.getValueRef<RT>() = b_distr(generator);
        b.setUBCode(UBKind::NoUB);
        ret = a << b;
        check_func(a, b, ret);
        ret = a >> b;
        check_func(a, b, ret);
    }

    // Lhs is negative
    if (std::is_signed<LT>::value) {
        b.getValueRef<RT>() = static_cast<RT>(a_distr(generator));
        b.setUBCode(UBKind::NoUB);
        a_distr = std::uniform_int_distribution<LT>(
            std::numeric_limits<LT>::min(), -1);
        a.getValueRef<LT>() = a_distr(generator);
        a.setUBCode(UBKind::NoUB);
        ret = a << b;
        check_func(a, b, ret);
        ret = a >> b;
        check_func(a, b, ret);
    }

    size_t lhs_bit_size = sizeof(LT) * CHAR_BIT;
    // Lhs is signed and Rhs is too large (moves msb(lhs) out of type
    // representation)
    if (std::is_signed<LT>::value) {
        a.getValueRef<LT>() = a_distr(generator);
        a.setUBCode(UBKind::NoUB);
        size_t max_avail_shft = lhs_bit_size - findMSB(a.getValueRef<LT>());
        b_distr = std::uniform_int_distribution<RT>(
            static_cast<RT>(max_avail_shft), std::numeric_limits<RT>::max());
        b.getValueRef<RT>() = b_distr(generator);
        b.setUBCode(UBKind::NoUB);
        ret = a << b;
        check_func(a, b, ret);
    }

    // Normal case
    a_distr =
        std::uniform_int_distribution<LT>(0, std::numeric_limits<LT>::max());
    a.getValueRef<LT>() = a_distr(generator);
    a.setUBCode(UBKind::NoUB);
    RT b_max = static_cast<RT>(lhs_bit_size) - 1;
    b_distr = std::uniform_int_distribution<RT>(0, b_max);
    b.getValueRef<RT>() = b_distr(generator);
    b.setUBCode(UBKind::NoUB);
    ret = a >> b;
    if (ret.getUBCode() != UBKind::NoUB ||
        ret.getValueRef<LT>() != (a.getValueRef<LT>() >> b.getValueRef<RT>())) {
        std::cout << "ERROR: " << __FUNCTION__ << typeid(LT).name();
        std::cout << "ERROR NoUB: " << a.getValueRef<LT>() << " | "
                  << b.getValueRef<RT>() << std::endl;
    }

    if (std::is_signed<LT>::value)
        b_max = static_cast<RT>(lhs_bit_size - findMSB(a.getValueRef<LT>()));
    b_distr = std::uniform_int_distribution<RT>(0, b_max);
    b.getValueRef<RT>() = b_distr(generator);
    b.setUBCode(UBKind::NoUB);

    ret = a << b;
    if (ret.getUBCode() != UBKind::NoUB ||
        ret.getValueRef<LT>() != (a.getValueRef<LT>() << b.getValueRef<RT>())) {
        std::cout << "ERROR: " << __FUNCTION__ << typeid(LT).name();
        std::cout << "ERROR NoUB: " << a.getValueRef<LT>() << " | "
                  << b.getValueRef<RT>() << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////

template <typename T> void singlePlusBitwiseNegateTest(IntTypeID type_id) {
    IRValue a(type_id);

    auto distr = getDistribution<T>();
    a.getValueRef<T>() = static_cast<T>(distr(generator));
    a.setUBCode(UBKind::NoUB);

    IRValue ret = +a;
    auto check_plus_func = [](T &a) -> T { return +a; };
    checkUnaryNoUB<T>(ret, a, check_plus_func, __FUNCTION__);

    ret = ~a;
    checkUnaryNoUB<T>(ret, a, std::bit_not<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T> void singleMinusTest(IntTypeID type_id) {
    IRValue a(type_id);

    auto bool_distr = getDistribution<bool>();
    auto distr = getDistribution<T>();
    a.getValueRef<T>() = static_cast<T>(distr(generator));
    a.setUBCode(UBKind::NoUB);

    auto test_ub = std::is_signed<T>::value && bool_distr(generator);

    if (test_ub) {
        a.getValueRef<T>() = std::numeric_limits<T>::min();
        a.setUBCode(UBKind::NoUB);
    }
    else {
        a.getValueRef<T>() = static_cast<T>(distr(generator));
        a.setUBCode(UBKind::NoUB);
    }

    IRValue ret = -a;

    if (test_ub)
        checkUnaryForUB<T>(ret, a, __FUNCTION__);
    else
        checkUnaryNoUB<T>(ret, a, std::negate<T>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

void singleLogicalNegateTest() {
    IRValue a(IntTypeID::BOOL);

    auto distr = getDistribution<bool>();
    a.getValueRef<bool>() = static_cast<bool>(distr(generator));
    a.setUBCode(UBKind::NoUB);

    IRValue ret = !a;
    checkUnaryNoUB<bool>(ret, a, std::logical_not<>(), __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////////

template <typename NT, typename OT>
void singleCastTest(IntTypeID to_type_id, IntTypeID from_type_id) {
    IRValue a(from_type_id);
    auto distr = getDistribution<OT>();
    a.getValueRef<OT>() = distr(generator);
    a.setUBCode(UBKind::NoUB);

    IRValue res = a.castToType(to_type_id);
    if (res.getUBCode() != UBKind::NoUB ||
        res.getValueRef<NT>() != static_cast<NT>(a.getValueRef<OT>())) {
        std::cout << "ERROR: " << __FUNCTION__ << " " << typeid(NT).name()
                  << typeid(OT).name();
        std::cout << " ERROR NoUB: " << a.getValueRef<OT>() << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////

void ir_value_test() {
    int test_num = 100000;

    for (int i = 0; i < test_num; ++i) {
        singleAddTest<TypeSInt::value_type>(IntTypeID::INT);
        singleAddTest<TypeUInt::value_type>(IntTypeID::UINT);
        singleAddTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singleAddTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleSubTest<TypeSInt::value_type>(IntTypeID::INT);
        singleSubTest<TypeUInt::value_type>(IntTypeID::UINT);
        singleSubTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singleSubTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleMulTest<TypeSInt::value_type>(IntTypeID::INT);
        singleMulTest<TypeUInt::value_type>(IntTypeID::UINT);
        singleMulTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singleMulTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleDivModTest<TypeSInt::value_type>(IntTypeID::INT);
        singleDivModTest<TypeUInt::value_type>(IntTypeID::UINT);
        singleDivModTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singleDivModTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleCmpTest<TypeSInt::value_type>(IntTypeID::INT);
        singleCmpTest<TypeUInt::value_type>(IntTypeID::UINT);
        singleCmpTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singleCmpTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleLogicalAndOrTest();
    }

    for (int i = 0; i < test_num; ++i) {
        singleBitwiseAndOrXorTest<TypeSInt::value_type>(IntTypeID::INT);
        singleBitwiseAndOrXorTest<TypeUInt::value_type>(IntTypeID::UINT);
        singleBitwiseAndOrXorTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singleBitwiseAndOrXorTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleLeftRightShiftTest<TypeSInt::value_type, TypeSInt::value_type>(
            IntTypeID::INT, IntTypeID::INT);
        singleLeftRightShiftTest<TypeSInt::value_type, TypeUInt::value_type>(
            IntTypeID::INT, IntTypeID::UINT);
        singleLeftRightShiftTest<TypeSInt::value_type, TypeSLLong::value_type>(
            IntTypeID::INT, IntTypeID::LLONG);
        singleLeftRightShiftTest<TypeSInt::value_type, TypeULLong::value_type>(
            IntTypeID::INT, IntTypeID::ULLONG);

        singleLeftRightShiftTest<TypeUInt::value_type, TypeSInt::value_type>(
            IntTypeID::UINT, IntTypeID::INT);
        singleLeftRightShiftTest<TypeUInt::value_type, TypeUInt::value_type>(
            IntTypeID::UINT, IntTypeID::UINT);
        singleLeftRightShiftTest<TypeUInt::value_type, TypeSLLong::value_type>(
            IntTypeID::UINT, IntTypeID::LLONG);
        singleLeftRightShiftTest<TypeUInt::value_type, TypeULLong::value_type>(
            IntTypeID::UINT, IntTypeID::ULLONG);

        singleLeftRightShiftTest<TypeSLLong::value_type, TypeSInt::value_type>(
            IntTypeID::LLONG, IntTypeID::INT);
        singleLeftRightShiftTest<TypeSLLong::value_type, TypeUInt::value_type>(
            IntTypeID::LLONG, IntTypeID::UINT);
        singleLeftRightShiftTest<TypeSLLong::value_type,
                                 TypeSLLong::value_type>(IntTypeID::LLONG,
                                                         IntTypeID::LLONG);
        singleLeftRightShiftTest<TypeSLLong::value_type,
                                 TypeULLong::value_type>(IntTypeID::LLONG,
                                                         IntTypeID::ULLONG);

        singleLeftRightShiftTest<TypeULLong::value_type, TypeSInt::value_type>(
            IntTypeID::ULLONG, IntTypeID::INT);
        singleLeftRightShiftTest<TypeULLong::value_type, TypeUInt::value_type>(
            IntTypeID::ULLONG, IntTypeID::UINT);
        singleLeftRightShiftTest<TypeULLong::value_type,
                                 TypeSLLong::value_type>(IntTypeID::ULLONG,
                                                         IntTypeID::LLONG);
        singleLeftRightShiftTest<TypeULLong::value_type,
                                 TypeULLong::value_type>(IntTypeID::ULLONG,
                                                         IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singlePlusBitwiseNegateTest<TypeSInt::value_type>(IntTypeID::INT);
        singlePlusBitwiseNegateTest<TypeUInt::value_type>(IntTypeID::UINT);
        singlePlusBitwiseNegateTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singlePlusBitwiseNegateTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleMinusTest<TypeSInt::value_type>(IntTypeID::INT);
        singleMinusTest<TypeUInt::value_type>(IntTypeID::UINT);
        singleMinusTest<TypeSLLong::value_type>(IntTypeID::LLONG);
        singleMinusTest<TypeULLong::value_type>(IntTypeID::ULLONG);
    }

    for (int i = 0; i < test_num; ++i) {
        singleLogicalNegateTest();
    }

    for (int i = 0; i < test_num; ++i) {
        singleCastTest<TypeBool::value_type, TypeBool::value_type>(
            IntTypeID::BOOL, IntTypeID::BOOL);
        singleCastTest<TypeBool::value_type, TypeSChar::value_type>(
            IntTypeID::BOOL, IntTypeID::SCHAR);
        singleCastTest<TypeBool::value_type, TypeUChar::value_type>(
            IntTypeID::BOOL, IntTypeID::UCHAR);
        singleCastTest<TypeBool::value_type, TypeSShort::value_type>(
            IntTypeID::BOOL, IntTypeID::SHORT);
        singleCastTest<TypeBool::value_type, TypeUShort::value_type>(
            IntTypeID::BOOL, IntTypeID::USHORT);
        singleCastTest<TypeBool::value_type, TypeSInt::value_type>(
            IntTypeID::BOOL, IntTypeID::INT);
        singleCastTest<TypeBool::value_type, TypeUInt::value_type>(
            IntTypeID::BOOL, IntTypeID::UINT);
        singleCastTest<TypeBool::value_type, TypeSLLong::value_type>(
            IntTypeID::BOOL, IntTypeID::LLONG);
        singleCastTest<TypeBool::value_type, TypeULLong::value_type>(
            IntTypeID::BOOL, IntTypeID::ULLONG);

        singleCastTest<TypeSChar::value_type, TypeBool::value_type>(
            IntTypeID::SCHAR, IntTypeID::BOOL);
        singleCastTest<TypeSChar::value_type, TypeSChar::value_type>(
            IntTypeID::SCHAR, IntTypeID::SCHAR);
        singleCastTest<TypeSChar::value_type, TypeUChar::value_type>(
            IntTypeID::SCHAR, IntTypeID::UCHAR);
        singleCastTest<TypeSChar::value_type, TypeSShort::value_type>(
            IntTypeID::SCHAR, IntTypeID::SHORT);
        singleCastTest<TypeSChar::value_type, TypeUShort::value_type>(
            IntTypeID::SCHAR, IntTypeID::USHORT);
        singleCastTest<TypeSChar::value_type, TypeSInt::value_type>(
            IntTypeID::SCHAR, IntTypeID::INT);
        singleCastTest<TypeSChar::value_type, TypeUInt::value_type>(
            IntTypeID::SCHAR, IntTypeID::UINT);
        singleCastTest<TypeSChar::value_type, TypeSLLong::value_type>(
            IntTypeID::SCHAR, IntTypeID::LLONG);
        singleCastTest<TypeSChar::value_type, TypeULLong::value_type>(
            IntTypeID::SCHAR, IntTypeID::ULLONG);

        singleCastTest<TypeUChar::value_type, TypeBool::value_type>(
            IntTypeID::SCHAR, IntTypeID::BOOL);
        singleCastTest<TypeUChar::value_type, TypeSChar::value_type>(
            IntTypeID::SCHAR, IntTypeID::SCHAR);
        singleCastTest<TypeUChar::value_type, TypeUChar::value_type>(
            IntTypeID::SCHAR, IntTypeID::UCHAR);
        singleCastTest<TypeUChar::value_type, TypeSShort::value_type>(
            IntTypeID::SCHAR, IntTypeID::SHORT);
        singleCastTest<TypeUChar::value_type, TypeUShort::value_type>(
            IntTypeID::SCHAR, IntTypeID::USHORT);
        singleCastTest<TypeUChar::value_type, TypeSInt::value_type>(
            IntTypeID::SCHAR, IntTypeID::INT);
        singleCastTest<TypeUChar::value_type, TypeUInt::value_type>(
            IntTypeID::SCHAR, IntTypeID::UINT);
        singleCastTest<TypeUChar::value_type, TypeSLLong::value_type>(
            IntTypeID::SCHAR, IntTypeID::LLONG);
        singleCastTest<TypeUChar::value_type, TypeULLong::value_type>(
            IntTypeID::SCHAR, IntTypeID::ULLONG);

        singleCastTest<TypeSShort::value_type, TypeBool::value_type>(
            IntTypeID::SHORT, IntTypeID::BOOL);
        singleCastTest<TypeSShort::value_type, TypeSChar::value_type>(
            IntTypeID::SHORT, IntTypeID::SCHAR);
        singleCastTest<TypeSShort::value_type, TypeUChar::value_type>(
            IntTypeID::SHORT, IntTypeID::UCHAR);
        singleCastTest<TypeSShort::value_type, TypeSShort::value_type>(
            IntTypeID::SHORT, IntTypeID::SHORT);
        singleCastTest<TypeSShort::value_type, TypeUShort::value_type>(
            IntTypeID::SHORT, IntTypeID::USHORT);
        singleCastTest<TypeSShort::value_type, TypeSInt::value_type>(
            IntTypeID::SHORT, IntTypeID::INT);
        singleCastTest<TypeSShort::value_type, TypeUInt::value_type>(
            IntTypeID::SHORT, IntTypeID::UINT);
        singleCastTest<TypeSShort::value_type, TypeSLLong::value_type>(
            IntTypeID::SHORT, IntTypeID::LLONG);
        singleCastTest<TypeSShort::value_type, TypeULLong::value_type>(
            IntTypeID::SHORT, IntTypeID::ULLONG);

        singleCastTest<TypeUShort::value_type, TypeBool::value_type>(
            IntTypeID::USHORT, IntTypeID::BOOL);
        singleCastTest<TypeUShort::value_type, TypeSChar::value_type>(
            IntTypeID::USHORT, IntTypeID::SCHAR);
        singleCastTest<TypeUShort::value_type, TypeUChar::value_type>(
            IntTypeID::USHORT, IntTypeID::UCHAR);
        singleCastTest<TypeUShort::value_type, TypeSShort::value_type>(
            IntTypeID::USHORT, IntTypeID::SHORT);
        singleCastTest<TypeUShort::value_type, TypeUShort::value_type>(
            IntTypeID::USHORT, IntTypeID::USHORT);
        singleCastTest<TypeUShort::value_type, TypeSInt::value_type>(
            IntTypeID::USHORT, IntTypeID::INT);
        singleCastTest<TypeUShort::value_type, TypeUInt::value_type>(
            IntTypeID::USHORT, IntTypeID::UINT);
        singleCastTest<TypeUShort::value_type, TypeSLLong::value_type>(
            IntTypeID::USHORT, IntTypeID::LLONG);
        singleCastTest<TypeUShort::value_type, TypeULLong::value_type>(
            IntTypeID::USHORT, IntTypeID::ULLONG);

        singleCastTest<TypeSInt::value_type, TypeBool::value_type>(
            IntTypeID::INT, IntTypeID::BOOL);
        singleCastTest<TypeSInt::value_type, TypeSChar::value_type>(
            IntTypeID::INT, IntTypeID::SCHAR);
        singleCastTest<TypeSInt::value_type, TypeUChar::value_type>(
            IntTypeID::INT, IntTypeID::UCHAR);
        singleCastTest<TypeSInt::value_type, TypeSShort::value_type>(
            IntTypeID::INT, IntTypeID::SHORT);
        singleCastTest<TypeSInt::value_type, TypeUShort::value_type>(
            IntTypeID::INT, IntTypeID::USHORT);
        singleCastTest<TypeSInt::value_type, TypeSInt::value_type>(
            IntTypeID::INT, IntTypeID::INT);
        singleCastTest<TypeSInt::value_type, TypeUInt::value_type>(
            IntTypeID::INT, IntTypeID::UINT);
        singleCastTest<TypeSInt::value_type, TypeSLLong::value_type>(
            IntTypeID::INT, IntTypeID::LLONG);
        singleCastTest<TypeSInt::value_type, TypeULLong::value_type>(
            IntTypeID::INT, IntTypeID::ULLONG);

        singleCastTest<TypeUInt::value_type, TypeBool::value_type>(
            IntTypeID::UINT, IntTypeID::BOOL);
        singleCastTest<TypeUInt::value_type, TypeSChar::value_type>(
            IntTypeID::UINT, IntTypeID::SCHAR);
        singleCastTest<TypeUInt::value_type, TypeUChar::value_type>(
            IntTypeID::UINT, IntTypeID::UCHAR);
        singleCastTest<TypeUInt::value_type, TypeSShort::value_type>(
            IntTypeID::UINT, IntTypeID::SHORT);
        singleCastTest<TypeUInt::value_type, TypeUShort::value_type>(
            IntTypeID::UINT, IntTypeID::USHORT);
        singleCastTest<TypeUInt::value_type, TypeSInt::value_type>(
            IntTypeID::UINT, IntTypeID::INT);
        singleCastTest<TypeUInt::value_type, TypeUInt::value_type>(
            IntTypeID::UINT, IntTypeID::UINT);
        singleCastTest<TypeUInt::value_type, TypeSLLong::value_type>(
            IntTypeID::UINT, IntTypeID::LLONG);
        singleCastTest<TypeUInt::value_type, TypeULLong::value_type>(
            IntTypeID::UINT, IntTypeID::ULLONG);

        singleCastTest<TypeSLLong::value_type, TypeBool::value_type>(
            IntTypeID::LLONG, IntTypeID::BOOL);
        singleCastTest<TypeSLLong::value_type, TypeSChar::value_type>(
            IntTypeID::LLONG, IntTypeID::SCHAR);
        singleCastTest<TypeSLLong::value_type, TypeUChar::value_type>(
            IntTypeID::LLONG, IntTypeID::UCHAR);
        singleCastTest<TypeSLLong::value_type, TypeSShort::value_type>(
            IntTypeID::LLONG, IntTypeID::SHORT);
        singleCastTest<TypeSLLong::value_type, TypeUShort::value_type>(
            IntTypeID::LLONG, IntTypeID::USHORT);
        singleCastTest<TypeSLLong::value_type, TypeSInt::value_type>(
            IntTypeID::LLONG, IntTypeID::INT);
        singleCastTest<TypeSLLong::value_type, TypeUInt::value_type>(
            IntTypeID::LLONG, IntTypeID::UINT);
        singleCastTest<TypeSLLong::value_type, TypeSLLong::value_type>(
            IntTypeID::LLONG, IntTypeID::LLONG);
        singleCastTest<TypeSLLong::value_type, TypeULLong::value_type>(
            IntTypeID::LLONG, IntTypeID::ULLONG);

        singleCastTest<TypeULLong::value_type, TypeBool::value_type>(
            IntTypeID::ULLONG, IntTypeID::BOOL);
        singleCastTest<TypeULLong::value_type, TypeSChar::value_type>(
            IntTypeID::ULLONG, IntTypeID::SCHAR);
        singleCastTest<TypeULLong::value_type, TypeUChar::value_type>(
            IntTypeID::ULLONG, IntTypeID::UCHAR);
        singleCastTest<TypeULLong::value_type, TypeSShort::value_type>(
            IntTypeID::ULLONG, IntTypeID::SHORT);
        singleCastTest<TypeULLong::value_type, TypeUShort::value_type>(
            IntTypeID::ULLONG, IntTypeID::USHORT);
        singleCastTest<TypeULLong::value_type, TypeSInt::value_type>(
            IntTypeID::ULLONG, IntTypeID::INT);
        singleCastTest<TypeULLong::value_type, TypeUInt::value_type>(
            IntTypeID::ULLONG, IntTypeID::UINT);
        singleCastTest<TypeULLong::value_type, TypeSLLong::value_type>(
            IntTypeID::ULLONG, IntTypeID::LLONG);
        singleCastTest<TypeULLong::value_type, TypeULLong::value_type>(
            IntTypeID::ULLONG, IntTypeID::ULLONG);
    }
}

int main() {
    auto seed = rd();
    std::cout << "Test seed: " << seed << std::endl;
    generator.seed(seed);

    ir_value_test();

    // TODO: we need to find a better way to test types
    type_test();

    /*
    // Hash collision check.
    // Make IntegralType::int_type_set public if you want to run it.
    uint64_t total_records = 0;
    uint64_t n = IntegralType::int_type_set.bucket_count();
    for (unsigned i=0; i<n; ++i) {
        std::cout << "bucket #" << i << " contains: ";
        for (auto it = IntegralType::int_type_set.begin(i);
    it!=IntegralType::int_type_set.end(i); ++it) { total_records++; std::cout <<
    "[" << static_cast<int>(it->first.int_type_id) << ":"
                      << it->second << "] ";
        }
            std::cout << "\n";
    }
    std::cout << "Total int record: " << total_records << std::endl;

    total_records = 0;
    n = ArrayType::array_type_set.bucket_count();
    for (unsigned i=0; i<n; ++i) {
        std::cout << "bucket #" << i << " contains: ";
        for (auto it = ArrayType::array_type_set.begin(i);
             it != ArrayType::array_type_set.end(i); ++it) {
            total_records++;
            std::cout << "["
                      << it->second << "] ";
        }
        std::cout << "\n";
    }
    std::cout << "Total array records: " << total_records << std::endl;
    */
}
