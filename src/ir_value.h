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

#include <cassert>
#include <climits>
#include <cstdint>
#include <functional>
#include <limits>

#include "enums.h"
#include "utils.h"

namespace yarpgen {

// This class represents all scalar values in Intermediate Representation
class IRValue {
  public:
    union Value {
        bool bool_val;
        int8_t schar_val;
        uint8_t uchar_val;
        int16_t shrt_val;
        uint16_t ushrt_val;
        int32_t int_val;
        uint32_t uint_val;
        int64_t llong_val;
        uint64_t ullong_val;
    };

    IRValue();

    explicit IRValue(IntTypeID _type_id);

    template <typename T> T &getValueRef();

    IntTypeID getIntTypeID() { return type_id; }
    UBKind getUBCode() { return ub_code; }
    void setUBCode(UBKind _ub_code) { ub_code = _ub_code; }
    void setIsUndefined(bool _undef) { undefined = _undef; }
    bool isUndefined() { return undefined; }

    // TODO: we need to add prefix and postfix operators
    IRValue operator+();
    IRValue operator-();
    IRValue operator~();
    IRValue operator!();

    IRValue operator+(IRValue &rhs);
    IRValue operator-(IRValue &rhs);
    IRValue operator*(IRValue &rhs);
    IRValue operator/(IRValue &rhs);
    IRValue operator%(IRValue &rhs);
    IRValue operator<(IRValue &rhs);
    IRValue operator>(IRValue &rhs);
    IRValue operator<=(IRValue &rhs);
    IRValue operator>=(IRValue &rhs);
    IRValue operator==(IRValue &rhs);
    IRValue operator!=(IRValue &rhs);
    IRValue operator&&(IRValue &rhs);
    IRValue operator||(IRValue &rhs);
    IRValue operator&(IRValue &rhs);
    IRValue operator|(IRValue &rhs);
    IRValue operator^(IRValue &rhs);
    IRValue operator<<(IRValue &rhs);
    IRValue operator>>(IRValue &rhs);

    // TODO: we need cast operator

  private:
    IntTypeID type_id;
    Value value;
    bool undefined;
    UBKind ub_code;
};

template <> bool &IRValue::getValueRef();
template <> int8_t &IRValue::getValueRef();
template <> uint8_t &IRValue::getValueRef();
template <> int16_t &IRValue::getValueRef();
template <> uint16_t &IRValue::getValueRef();
template <> int32_t &IRValue::getValueRef();
template <> uint32_t &IRValue::getValueRef();
template <> int64_t &IRValue::getValueRef();
template <> uint64_t &IRValue::getValueRef();

//////////////////////////////////////////////////////////////////////////////
// These are defines that dispatch the appropriate template instantiation

// clang-format off
#define OperatorWrapperCase(__type_id__, __foo__)                              \
    case (__type_id__): func = __foo__; break;

#define OperatorWrapper(__foo__)                                               \
    do {                                                                       \
        switch (type_id) {                                                     \
            OperatorWrapperCase(IntTypeID::INT,  __foo__<TypeSInt::value_type>)\
            OperatorWrapperCase(IntTypeID::UINT,                               \
                                __foo__<TypeUInt::value_type>)                 \
            OperatorWrapperCase(IntTypeID::LLONG,                              \
                                __foo__<TypeSLLong::value_type>)               \
            OperatorWrapperCase(IntTypeID::ULLONG,                             \
                                __foo__<TypeULLong::value_type>)               \
            default: ERROR(std::string("Bad IntTypeID value: ") +              \
                           std::to_string(static_cast<int>(type_id)));         \
        }                                                                      \
    } while (0)

#define ShiftOperatorWrapperCaseCase(__type_id__, __foo__, __lhs_value_type__, \
                                     __rhs_value_type__)                       \
    case (__type_id__):                                                        \
        func = __foo__<__lhs_value_type__, __rhs_value_type__>;                \
        break;

#define ShiftOperatorWrapperCase(__type_id__, __foo__, __lhs_value_type__)     \
    case __type_id__:                                                          \
        switch (rhs.getIntTypeID()) {                                          \
            ShiftOperatorWrapperCaseCase(IntTypeID::INT, __foo__,              \
                                         __lhs_value_type__,                   \
                                         TypeSInt::value_type)                 \
            ShiftOperatorWrapperCaseCase(IntTypeID::UINT, __foo__,             \
                                         __lhs_value_type__,                   \
                                         TypeUInt::value_type)                 \
            ShiftOperatorWrapperCaseCase(IntTypeID::LLONG, __foo__,            \
                                         __lhs_value_type__,                   \
                                         TypeSLLong::value_type)               \
            ShiftOperatorWrapperCaseCase(IntTypeID::ULLONG, __foo__,           \
                                         __lhs_value_type__,                   \
                                         TypeULLong::value_type)               \
            default: ERROR(std::string("Bad IntTypeID value: ") +              \
                           std::to_string(static_cast<int>(type_id)));         \
        }                                                                      \
        break;

#define ShiftOperatorWrapper(__foo__)                                          \
    do {                                                                       \
        switch (type_id) {                                                     \
            ShiftOperatorWrapperCase(IntTypeID::INT, __foo__,                  \
                                     TypeSInt::value_type)                     \
            ShiftOperatorWrapperCase(IntTypeID::UINT, __foo__,                 \
                                     TypeUInt::value_type)                     \
            ShiftOperatorWrapperCase(IntTypeID::LLONG, __foo__,                \
                                     TypeSLLong::value_type)                   \
            ShiftOperatorWrapperCase(IntTypeID::ULLONG, __foo__,               \
                                     TypeULLong::value_type)                   \
            default: ERROR(std::string("Bad IntTypeID value: ") +              \
                           std::to_string(static_cast<int>(type_id)));         \
        }                                                                      \
    } while (0)

// clang-format on

//////////////////////////////////////////////////////////////////////////////
// The majority of operators can be implemented the same way.
// The only exception if they work only for single type.

#define UnaryOperatorImpl(__foo__)                                             \
    std::function<IRValue(IRValue &)> func;                                    \
    OperatorWrapper(__foo__);                                                  \
    return IRValue(func(*this));

#define BinaryOperatorImpl(__foo__)                                            \
    std::function<IRValue(IRValue &, IRValue &)> func;                         \
    OperatorWrapper(__foo__);                                                  \
    return IRValue(func(*this, rhs));

#define ShiftOperatorImpl(__foo__)                                             \
    std::function<IRValue(IRValue &, IRValue &)> func;                         \
    ShiftOperatorWrapper(__foo__);                                             \
    return IRValue(func(*this, rhs));

//////////////////////////////////////////////////////////////////////////////
// Find the most significant bit
// We also need this function to form test input, that's why it is here.
template <typename T> inline size_t findMSB(T x) {
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

} // namespace yarpgen
