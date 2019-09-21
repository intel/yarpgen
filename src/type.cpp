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

#include "type.h"
#include "utils.h"

using namespace yarpgen;

std::shared_ptr<IntegralType> yarpgen::IntegralType::init(yarpgen::IntegralType::IntTypeID _type_id) {
    return init(_type_id, false, CVQualifier::NONE);
}

std::shared_ptr<IntegralType>
IntegralType::init(IntegralType::IntTypeID _type_id, bool _is_static, Type::CVQualifier _cv_qual) {
    std::shared_ptr<IntegralType> ret;
    switch (_type_id) {
        case IntTypeID::BOOL:
            ret = std::make_shared<TypeBool>(TypeBool(_is_static, _cv_qual));
            break;
        case IntTypeID::SCHAR:
            ret = std::make_shared<TypeSChar>(TypeSChar(_is_static, _cv_qual));
            break;
        case IntTypeID::MAX_INT_TYPE_ID:
        //TODO: remove it later to create error for unhandled cases
        default:
            ERROR("Unsupported IntTypeID");
    }

    return ret;
}

static void dbgDumpHelper(IntegralType::IntTypeID id, const std::string& name, const std::string& suffix,
                          uint32_t bit_size, bool is_signed, uint64_t& min, uint64_t& max, bool is_static,
                          ArithmeticType::CVQualifier cv_qual) {
    std::cout << "int type id:  " << static_cast<std::underlying_type<IntegralType::IntTypeID>::type>(id) << std::endl;
    std::cout << "name:         " << name << std::endl;
    std::cout << "bit_size:     " << bit_size << std::endl;
    std::cout << "is_signed:    " << is_signed << std::endl;
    std::cout << "min:          " << min << suffix << std::endl;
    std::cout << "max:          " << max << suffix << std::endl;
    std::cout << "is_static:    " << is_static << std::endl;
    std::cout << "cv_qualifier: " <<
        static_cast<std::underlying_type<ArithmeticType::CVQualifier>::type>(cv_qual) << std::endl;
}

#define DBG_DUMP_MACROS(type_name) void type_name::dbgDump() { \
    dbgDumpHelper(getIntTypeId(), getName(), getLiteralSuffix(), getBitSize(), getIsSigned(), \
                  min.value, max.value, \
                  getIsStatic(), getCVQualifier()); \
}

DBG_DUMP_MACROS(TypeBool)
DBG_DUMP_MACROS(TypeSChar)

