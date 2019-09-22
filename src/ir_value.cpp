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

template <> bool &IRValue::getValueRef() { return value.bool_val; }
IRValue::IRValue()
    : type_id(IntTypeID::MAX_INT_TYPE_ID), undefined(true),
      ub_code(UBKind::NoUB) {
    value.ullong_val = 0;
}

IRValue::IRValue(IntTypeID _type_id)
    : type_id(_type_id), undefined(true), ub_code(UBKind::NoUB) {
    value.ullong_val = 0;
}

template <> signed char &IRValue::getValueRef() { return value.schar_val; }

template <> unsigned char &IRValue::getValueRef() { return value.uchar_val; }

template <> short &IRValue::getValueRef() { return value.shrt_val; }

template <> unsigned short &IRValue::getValueRef() { return value.ushrt_val; }

template <> int &IRValue::getValueRef() { return value.int_val; }

template <> unsigned int &IRValue::getValueRef() { return value.uint_val; }

template <> long long int &IRValue::getValueRef() { return value.llong_val; }

template <> unsigned long long int &IRValue::getValueRef() {
    return value.ullong_val;
}
