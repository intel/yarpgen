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

#include "ir_value.h"

using namespace yarpgen;

IRValue::IRValue()
    : type_id(IntTypeID::MAX_INT_TYPE_ID), undefined(true),
      ub_code(UBKind::NoUB) {
    value.ullong_val = 0;
}

IRValue::IRValue(IntTypeID _type_id)
    : type_id(_type_id), undefined(true), ub_code(UBKind::NoUB) {
    value.ullong_val = 0;
}

template <> bool &IRValue::getValueRef() { return value.bool_val; }

template <> int8_t &IRValue::getValueRef() { return value.schar_val; }

template <> uint8_t &IRValue::getValueRef() { return value.uchar_val; }

template <> int16_t &IRValue::getValueRef() { return value.shrt_val; }

template <> uint16_t &IRValue::getValueRef() { return value.ushrt_val; }

template <> int32_t &IRValue::getValueRef() { return value.int_val; }

template <> uint32_t &IRValue::getValueRef() { return value.uint_val; }

template <> int64_t &IRValue::getValueRef() { return value.llong_val; }

template <> uint64_t &IRValue::getValueRef() { return value.ullong_val; }