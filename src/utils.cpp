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

#include "utils.h"
#include "type.h"
#include <memory>

using namespace yarpgen;

std::shared_ptr<RandValGen> yarpgen::rand_val_gen;

RandValGen::RandValGen(uint64_t _seed) {
    if (_seed != 0) {
        seed = _seed;
    }
    else {
        std::random_device rd;
        seed = rd();
    }
    std::cout << "/*SEED " << seed << "*/" << std::endl;
    rand_gen = std::mt19937_64(seed);
}

#define RandValueCase(__type_id__, type_name)                                  \
    case __type_id__:                                                          \
        do {                                                                   \
            ret.getValueRef<type_name>() = getRandValue<type_name>();          \
            break;                                                             \
        } while (false)

IRValue RandValGen::getRandValue(IntTypeID type_id) {
    if (type_id == IntTypeID::MAX_INT_TYPE_ID)
        ERROR("Bad IntTypeID");

    IRValue ret(type_id);
    switch (type_id) {
        // TODO: if we use chains of if we can make it simpler
        RandValueCase(IntTypeID::BOOL, TypeBool::value_type);
        RandValueCase(IntTypeID::SCHAR, TypeSChar::value_type);
        RandValueCase(IntTypeID::UCHAR, TypeUChar::value_type);
        RandValueCase(IntTypeID::SHORT, TypeSShort::value_type);
        RandValueCase(IntTypeID::USHORT, TypeUShort::value_type);
        RandValueCase(IntTypeID::INT, TypeSInt::value_type);
        RandValueCase(IntTypeID::UINT, TypeUInt::value_type);
        RandValueCase(IntTypeID::LLONG, TypeSLLong::value_type);
        RandValueCase(IntTypeID::ULLONG, TypeULLong::value_type);
        case IntTypeID::MAX_INT_TYPE_ID:
            ERROR("Bad IntTypeID");
            break;
    }
    return ret;
}