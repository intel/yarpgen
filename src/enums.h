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

namespace yarpgen {

// All possible Integral Types, used as a backend type
enum class IntTypeID {
    BOOL,
    // We don't have "char" as a type, because its signedness is
    // implementation-defined
    SCHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    // We don't have any kind of "long" types,
    // because their real size depends on the architecture
    LLONG,
    ULLONG,
    MAX_INT_TYPE_ID
};

enum class CVQualifier {
    NONE,
    CONST,
    VOLAT,
    CONST_VOLAT,
};

// All possible cases of Undefined Behaviour.
// For now we treat implementation-defined behaviour as undefined behaviour
// TODO: do we want to allow implementation-defined behaviour?
enum class UBKind {
    NoUB,
    // NullPtr,       // nullptr ptr dereference
    SignOvf,       // Signed overflow
    SignOvfMin,    // Special case of signed overflow: INT_MIN * (-1)
    ZeroDiv,       // FPE
    ShiftRhsNeg,   // Shift by negative value
    ShiftRhsLarge, // Shift by large value
    NegShift,      // Shift of negative value
    NoMemeber,     // Can't find member of structure
    MaxUB
};

} // namespace yarpgen
