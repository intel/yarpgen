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
#include "enums.h"
#include "ir_value.h"
#include "utils.h"

using namespace yarpgen;

std::unordered_map<IntTypeKey, std::shared_ptr<IntegralType>, IntTypeKeyHasher>
    yarpgen::IntegralType::int_type_set;

IntTypeKey::IntTypeKey(IntTypeID _int_type_id, bool _is_static,
                       CVQualifier _cv_qualifier)
    : int_type_id(_int_type_id), is_static(_is_static),
      cv_qualifier(_cv_qualifier) {}

bool IntTypeKey::operator==(const IntTypeKey &other) const {
    return (int_type_id == other.int_type_id) &&
           (is_static == other.is_static) &&
           (cv_qualifier == other.cv_qualifier);
}

std::size_t IntTypeKeyHasher::operator()(const IntTypeKey &key) const {
    // TODO: we have a collisions. We need to think about better hash-function
    std::size_t hash_seed = 17;
    using under_type_of_id =
        std::underlying_type<decltype(key.int_type_id)>::type;
    using under_type_of_cv_qual =
        std::underlying_type<decltype(key.cv_qualifier)>::type;

    size_t first_hash = std::hash<under_type_of_id>()(
        static_cast<under_type_of_id>(key.int_type_id));
    size_t second_hash = std::hash<decltype(key.is_static)>()(key.is_static);
    size_t third_hash = std::hash<under_type_of_cv_qual>()(
        static_cast<under_type_of_cv_qual>(key.cv_qualifier));

    hashCombine(first_hash, hash_seed);
    hashCombine(second_hash, hash_seed);
    hashCombine(third_hash, hash_seed);
    return hash_seed;
}

size_t IntTypeKeyHasher::hashCombine(size_t &value, size_t &seed) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

std::shared_ptr<IntegralType> yarpgen::IntegralType::init(IntTypeID _type_id) {
    return init(_type_id, false, CVQualifier::NONE);
}

std::shared_ptr<IntegralType>
IntegralType::init(IntTypeID _type_id, bool _is_static, CVQualifier _cv_qual) {
    // Folding set lookup
    IntTypeKey key(_type_id, _is_static, _cv_qual);
    auto find_result = int_type_set.find(key);
    if (find_result != int_type_set.end())
        return find_result->second;

    // Create type if we can't find it
    std::shared_ptr<IntegralType> ret;
    switch (_type_id) {
    case IntTypeID::BOOL:
        ret = std::make_shared<TypeBool>(TypeBool(_is_static, _cv_qual));
        break;
    case IntTypeID::SCHAR:
        ret = std::make_shared<TypeSChar>(TypeSChar(_is_static, _cv_qual));
        break;
    case IntTypeID::UCHAR:
        ret = std::make_shared<TypeUChar>(TypeUChar(_is_static, _cv_qual));
        break;
    case IntTypeID::SHORT:
        ret = std::make_shared<TypeSShort>(TypeSShort(_is_static, _cv_qual));
        break;
    case IntTypeID::USHORT:
        ret = std::make_shared<TypeUShort>(TypeUShort(_is_static, _cv_qual));
        break;
    case IntTypeID::INT:
        ret = std::make_shared<TypeSInt>(TypeSInt(_is_static, _cv_qual));
        break;
    case IntTypeID::UINT:
        ret = std::make_shared<TypeUInt>(TypeUInt(_is_static, _cv_qual));
        break;
    case IntTypeID::LLONG:
        ret = std::make_shared<TypeSLLong>(TypeSLLong(_is_static, _cv_qual));
        break;
    case IntTypeID::ULLONG:
        ret = std::make_shared<TypeULLong>(TypeULLong(_is_static, _cv_qual));
        break;
    case IntTypeID::MAX_INT_TYPE_ID:
        ERROR("Unsupported IntTypeID");
    }

    int_type_set[key] = ret;
    return ret;
}

template <typename T>
static void dbgDumpHelper(IntTypeID id, const std::string &name,
                          const std::string &suffix, uint32_t bit_size,
                          bool is_signed, T &min, T &max, bool is_static,
                          CVQualifier cv_qual) {
    std::cout << "int type id:  "
              << static_cast<std::underlying_type<IntTypeID>::type>(id)
              << std::endl;
    std::cout << "name:         " << name << std::endl;
    std::cout << "bit_size:     " << bit_size << std::endl;
    std::cout << "is_signed:    " << is_signed << std::endl;
    std::cout << "min:          " << min << suffix << std::endl;
    std::cout << "max:          " << max << suffix << std::endl;
    std::cout << "is_static:    " << is_static << std::endl;
    std::cout << "cv_qualifier: "
              << static_cast<std::underlying_type<CVQualifier>::type>(cv_qual)
              << std::endl;
}

#define DBG_DUMP_MACROS(type_name)                                             \
    void type_name::dbgDump() {                                                \
        dbgDumpHelper(                                                         \
            getIntTypeId(), getName(), getLiteralSuffix(), getBitSize(),       \
            getIsSigned(), min.getValueRef<value_type>(),                      \
            max.getValueRef<value_type>(), getIsStatic(), getCVQualifier());   \
    }

DBG_DUMP_MACROS(TypeBool)
DBG_DUMP_MACROS(TypeSChar)
DBG_DUMP_MACROS(TypeUChar)
DBG_DUMP_MACROS(TypeSShort)
DBG_DUMP_MACROS(TypeUShort)
DBG_DUMP_MACROS(TypeSInt)
DBG_DUMP_MACROS(TypeUInt)
DBG_DUMP_MACROS(TypeSLLong)
DBG_DUMP_MACROS(TypeULLong)
