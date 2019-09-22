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
#include <cstdint>

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

} // namespace yarpgen
