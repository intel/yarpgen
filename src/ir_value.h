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

    struct AbsValue {
        bool isNegative;
        uint64_t value;

        bool operator==(const AbsValue &b) const {
            return isNegative == b.isNegative && value == b.value;
        }
    };

    IRValue();
    explicit IRValue(IntTypeID _type_id);
    IRValue(IntTypeID _type_id, AbsValue _val);

    template <typename T> T &getValueRef();

    IntTypeID getIntTypeID() { return type_id; }
    UBKind getUBCode() { return ub_code; }
    void setUBCode(UBKind _ub_code) { ub_code = _ub_code; }
    bool hasUB() { return ub_code != UBKind::NoUB; }

    // TODO: we need to add prefix and postfix operators
    IRValue operator+();
    IRValue operator-();
    IRValue operator~();
    IRValue operator!();

    friend IRValue operator+(IRValue lhs, IRValue rhs);
    friend IRValue operator-(IRValue lhs, IRValue rhs);
    friend IRValue operator*(IRValue lhs, IRValue rhs);
    friend IRValue operator/(IRValue lhs, IRValue rhs);
    friend IRValue operator%(IRValue lhs, IRValue rhs);
    friend IRValue operator<(IRValue lhs, IRValue rhs);
    friend IRValue operator>(IRValue lhs, IRValue rhs);
    friend IRValue operator<=(IRValue lhs, IRValue rhs);
    friend IRValue operator>=(IRValue lhs, IRValue rhs);
    friend IRValue operator==(IRValue lhs, IRValue rhs);
    friend IRValue operator!=(IRValue lhs, IRValue rhs);
    friend IRValue operator&&(IRValue lhs, IRValue rhs);
    friend IRValue operator||(IRValue lhs, IRValue rhs);
    friend IRValue operator&(IRValue lhs, IRValue rhs);
    friend IRValue operator|(IRValue lhs, IRValue rhs);
    friend IRValue operator^(IRValue lhs, IRValue rhs);
    friend IRValue operator<<(IRValue lhs, IRValue rhs);
    friend IRValue operator>>(IRValue lhs, IRValue rhs);

    IRValue castToType(IntTypeID to_type);

    friend std::ostream &operator<<(std::ostream &out, IRValue &val);

    size_t getMSB();

    AbsValue getAbsValue();
    void setValue(AbsValue val);

  private:
    IntTypeID type_id;
    Value value;
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

#define OperatorWrapper(__foo__, __type_id__)                                  \
    do {                                                                       \
        switch (__type_id__) {                                                 \
            OperatorWrapperCase(IntTypeID::INT,  __foo__<TypeSInt::value_type>)\
            OperatorWrapperCase(IntTypeID::UINT,                               \
                                __foo__<TypeUInt::value_type>)                 \
            OperatorWrapperCase(IntTypeID::LLONG,                              \
                                __foo__<TypeSLLong::value_type>)               \
            OperatorWrapperCase(IntTypeID::ULLONG,                             \
                                __foo__<TypeULLong::value_type>)               \
            default: ERROR(std::string("Bad IntTypeID value: ") +              \
                           std::to_string(static_cast<int>(__type_id__)));     \
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
                           std::to_string(static_cast<int>(rhs.getIntTypeID())));\
        }                                                                      \
        break;

#define ShiftOperatorWrapper(__foo__)                                          \
    do {                                                                       \
        switch (lhs.getIntTypeID()) {                                          \
            ShiftOperatorWrapperCase(IntTypeID::INT, __foo__,                  \
                                     TypeSInt::value_type)                     \
            ShiftOperatorWrapperCase(IntTypeID::UINT, __foo__,                 \
                                     TypeUInt::value_type)                     \
            ShiftOperatorWrapperCase(IntTypeID::LLONG, __foo__,                \
                                     TypeSLLong::value_type)                   \
            ShiftOperatorWrapperCase(IntTypeID::ULLONG, __foo__,               \
                                     TypeULLong::value_type)                   \
            default: ERROR(std::string("Bad IntTypeID value: ") +              \
                           std::to_string(static_cast<int>(lhs.getIntTypeID())));\
        }                                                                      \
    } while (0)

#define CastOperatorWrapperCaseCase(__type_id__, __foo__, __to_value_type__,   \
                                     __from_value_type__)                      \
    case (__type_id__):                                                        \
        func = __foo__<__to_value_type__, __from_value_type__>;                \
        break;

#define CastOperatorWrapperCase(__type_id__, __foo__, __to_value_type__)       \
    case __type_id__:                                                          \
        switch (type_id) {                                                     \
            CastOperatorWrapperCaseCase(IntTypeID::BOOL, __foo__,              \
                                         __to_value_type__,                    \
                                         TypeBool::value_type)                 \
            CastOperatorWrapperCaseCase(IntTypeID::SCHAR, __foo__,             \
                                         __to_value_type__,                    \
                                         TypeSChar::value_type)                \
            CastOperatorWrapperCaseCase(IntTypeID::UCHAR, __foo__,             \
                                         __to_value_type__,                    \
                                         TypeUChar::value_type)                \
            CastOperatorWrapperCaseCase(IntTypeID::SHORT, __foo__,             \
                                         __to_value_type__,                    \
                                         TypeSShort::value_type)               \
            CastOperatorWrapperCaseCase(IntTypeID::USHORT, __foo__,            \
                                         __to_value_type__,                    \
                                         TypeUShort::value_type)               \
            CastOperatorWrapperCaseCase(IntTypeID::INT, __foo__,               \
                                         __to_value_type__,                    \
                                         TypeSInt::value_type)                 \
            CastOperatorWrapperCaseCase(IntTypeID::UINT, __foo__,              \
                                         __to_value_type__,                    \
                                         TypeUInt::value_type)                 \
            CastOperatorWrapperCaseCase(IntTypeID::LLONG, __foo__,             \
                                         __to_value_type__,                    \
                                         TypeSLLong::value_type)               \
            CastOperatorWrapperCaseCase(IntTypeID::ULLONG, __foo__,            \
                                         __to_value_type__,                    \
                                         TypeULLong::value_type)               \
            default: ERROR(std::string("Bad IntTypeID value: ") +              \
                           std::to_string(static_cast<int>(type_id)));         \
        }                                                                      \
        break;

#define CastOperatorWrapper(__foo__)                                           \
    do {                                                                       \
        switch (to_type_id) {                                                  \
            CastOperatorWrapperCase(IntTypeID::BOOL, __foo__,                  \
                                     TypeBool::value_type)                     \
            CastOperatorWrapperCase(IntTypeID::SCHAR, __foo__,                 \
                                     TypeSChar::value_type)                    \
            CastOperatorWrapperCase(IntTypeID::UCHAR, __foo__,                 \
                                     TypeUChar::value_type)                    \
            CastOperatorWrapperCase(IntTypeID::SHORT, __foo__,                 \
                                     TypeSShort::value_type)                   \
            CastOperatorWrapperCase(IntTypeID::USHORT, __foo__,                \
                                     TypeUShort::value_type)                   \
            CastOperatorWrapperCase(IntTypeID::INT, __foo__,                   \
                                     TypeSInt::value_type)                     \
            CastOperatorWrapperCase(IntTypeID::UINT, __foo__,                  \
                                     TypeUInt::value_type)                     \
            CastOperatorWrapperCase(IntTypeID::LLONG, __foo__,                 \
                                     TypeSLLong::value_type)                   \
            CastOperatorWrapperCase(IntTypeID::ULLONG, __foo__,                \
                                     TypeULLong::value_type)                   \
            default: ERROR(std::string("Bad IntTypeID value: ") +              \
                           std::to_string(static_cast<int>(type_id)));         \
        }                                                                      \
    } while (0)

#define OutOperatorCase(__type_id__, __type__)                                 \
    case (__type_id__):                                                        \
        out << std::to_string(val.getValueRef<__type__>());                    \
        break;

#define GetMSBCase(__type_id__, __type__)                                      \
    case (__type_id__): return getMSBImpl(getValueRef<__type__>());

// clang-format on

//////////////////////////////////////////////////////////////////////////////
// The majority of operators can be implemented the same way.
// The only exception if they work only for single type.

#define UnaryOperatorImpl(__foo__)                                             \
    do {                                                                       \
        std::function<IRValue(IRValue &)> func;                                \
        OperatorWrapper(__foo__, getIntTypeID());                              \
        return {func(*this)};                                                  \
    } while (0)

#define BinaryOperatorImpl(__foo__)                                            \
    do {                                                                       \
        std::function<IRValue(IRValue &, IRValue &)> func;                     \
        OperatorWrapper(__foo__, lhs.getIntTypeID());                          \
        return {func(lhs, rhs)};                                               \
    } while (0)

#define ShiftOperatorImpl(__foo__)                                             \
    do {                                                                       \
        std::function<IRValue(IRValue &, IRValue &)> func;                     \
        ShiftOperatorWrapper(__foo__);                                         \
        return {func(lhs, rhs)};                                               \
    } while (0)

//////////////////////////////////////////////////////////////////////////////

std::ostream &operator<<(std::ostream &out, yarpgen::IRValue &val);
// TODO: ideally, rhs should have a const IRValue&, but it causes problem with
// getValueRef
IRValue operator+(IRValue lhs, IRValue rhs);
IRValue operator-(IRValue lhs, IRValue rhs);
IRValue operator*(IRValue lhs, IRValue rhs);
IRValue operator/(IRValue lhs, IRValue rhs);
IRValue operator%(IRValue lhs, IRValue rhs);
IRValue operator<(IRValue lhs, IRValue rhs);
IRValue operator>(IRValue lhs, IRValue rhs);
IRValue operator<=(IRValue lhs, IRValue rhs);
IRValue operator>=(IRValue lhs, IRValue rhs);
IRValue operator==(IRValue lhs, IRValue rhs);
IRValue operator!=(IRValue lhs, IRValue rhs);
IRValue operator&&(IRValue lhs, IRValue rhs);
IRValue operator||(IRValue lhs, IRValue rhs);
IRValue operator&(IRValue lhs, IRValue rhs);
IRValue operator|(IRValue lhs, IRValue rhs);
IRValue operator^(IRValue lhs, IRValue rhs);
IRValue operator<<(IRValue lhs, IRValue rhs);
IRValue operator>>(IRValue lhs, IRValue rhs);

} // namespace yarpgen
