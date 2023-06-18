/*
Copyright (c) 2015-2020, Intel Corporation
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

#include "context.h"
#include <utility>

#include "data.h"
#include "enums.h"
#include "expr.h"
#include "ir_value.h"
#include "type.h"
#include "utils.h"

using namespace yarpgen;

std::unordered_map<IntTypeKey, std::shared_ptr<IntegralType>, IntTypeKeyHasher>
    yarpgen::IntegralType::int_type_set;

std::unordered_map<ArrayTypeKey, std::shared_ptr<ArrayType>, ArrayTypeKeyHasher>
    yarpgen::ArrayType::array_type_set;
size_t yarpgen::ArrayType::uid_counter = 0;

std::shared_ptr<IntegralType> yarpgen::IntegralType::init(IntTypeID _type_id) {
    return init(_type_id, false, CVQualifier::NONE);
}

std::shared_ptr<IntegralType> IntegralType::init(IntTypeID _type_id,
                                                 bool _is_static,
                                                 CVQualifier _cv_qual,
                                                 bool _is_uniform) {
    // Folding set lookup
    IntTypeKey key(_type_id, _is_static, _cv_qual, _is_uniform);
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
            ret =
                std::make_shared<TypeSShort>(TypeSShort(_is_static, _cv_qual));
            break;
        case IntTypeID::USHORT:
            ret =
                std::make_shared<TypeUShort>(TypeUShort(_is_static, _cv_qual));
            break;
        case IntTypeID::INT:
            ret = std::make_shared<TypeSInt>(TypeSInt(_is_static, _cv_qual));
            break;
        case IntTypeID::UINT:
            ret = std::make_shared<TypeUInt>(TypeUInt(_is_static, _cv_qual));
            break;
        case IntTypeID::LLONG:
            ret =
                std::make_shared<TypeSLLong>(TypeSLLong(_is_static, _cv_qual));
            break;
        case IntTypeID::ULLONG:
            ret =
                std::make_shared<TypeULLong>(TypeULLong(_is_static, _cv_qual));
            break;
        case IntTypeID::MAX_INT_TYPE_ID:
            ERROR("Unsupported IntTypeID");
    }

    ret->setIsUniform(_is_uniform);

    int_type_set[key] = ret;
    return ret;
}

bool IntegralType::isSame(std::shared_ptr<IntegralType> &lhs,
                          std::shared_ptr<IntegralType> &rhs) {
    return (lhs->getIntTypeId() == rhs->getIntTypeId()) &&
           (lhs->getIsStatic() == rhs->getIsStatic()) &&
           (lhs->getCVQualifier() == rhs->getCVQualifier() &&
            lhs->isUniform() == rhs->isUniform());
}

// Check if type "b" can represent all the values of type "a"
bool IntegralType::canRepresentType(IntTypeID a, IntTypeID b) {
    // This functions should be called only after integral promotions, so we
    // don't care about "small" types. Unfortunately, there is no way to enforce
    // it, so in case of violation it should fail generated tests.
    // TODO: can we do something about it?

    if (a == IntTypeID::INT && b == IntTypeID::LLONG)
        return true;

    if (a == IntTypeID::UINT &&
        (b == IntTypeID::ULLONG || b == IntTypeID::LLONG))
        return true;

    return false;
}

IntTypeID IntegralType::getCorrUnsigned(IntTypeID id) {
    // This functions should be called only after integral promotions, so we
    // don't care about "small" types.
    switch (id) {
        case IntTypeID::INT:
        case IntTypeID::UINT:
            return IntTypeID::UINT;
        case IntTypeID::LLONG:
        case IntTypeID::ULLONG:
            return IntTypeID::ULLONG;
        default:
            ERROR(
                "This function should be called only after IntegralPromotions");
    }
}
std::shared_ptr<Type> IntegralType::makeVarying() {
    return init(getIntTypeId(), getIsStatic(), getCVQualifier(), false);
}

std::string IntegralType::getNameImpl(std::shared_ptr<EmitCtx> ctx,
                                      std::string raw_name) {
    if (!ctx)
        ERROR("Can't give a name without a context");

    std::string ret = std::move(raw_name);
    if (ctx->useIspcTypes()) {
        ret = getIspcNameHelper();
        if (getIntTypeId() != IntTypeID::BOOL) {
            if (!getIsSigned())
                ret += "unsigned ";
            ret += "int" + std::to_string(getBitSize());
        }
        else
            ret += "bool";
    }
    return ret;
}

std::string IntegralType::getLiteralSuffix() {
    Options &options = Options::getInstance();
    return !options.isISPC() ? "" : "L";
}

template <typename T>
static void dbgDumpHelper(IntTypeID id, const std::string &name,
                          const std::string &suffix, size_t bit_size,
                          bool is_signed, T &min, T &max, bool is_static,
                          CVQualifier cv_qual, bool is_uniform) {
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
    std::cout << "is_uniform:   " << is_uniform << std::endl;
}

#define DBG_DUMP_MACROS(type_name)                                             \
    void type_name::dbgDump() {                                                \
        auto ctx = std::make_shared<EmitCtx>();                                \
        dbgDumpHelper(getIntTypeId(), getName(ctx), getLiteralSuffix(),        \
                      getBitSize(), getIsSigned(),                             \
                      min.getValueRef<value_type>(),                           \
                      max.getValueRef<value_type>(), getIsStatic(),            \
                      getCVQualifier(), isUniform());                          \
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

bool ArrayType::isSame(const std::shared_ptr<ArrayType> &lhs,
                       const std::shared_ptr<ArrayType> &rhs) {
    return lhs->getUID() == rhs->getUID();
}

void ArrayType::dbgDump() {
    std::cout << "Array: " << std::endl;
    std::cout << "Base type: " << std::endl;
    base_type->dbgDump();
    std::cout << "Dimensions: [";
    for (const auto &dim : dimensions) {
        std::cout << dim << ", ";
    }
    std::cout << "]" << std::endl;

    std::cout << "Kind: " << static_cast<int>(kind) << std::endl;
    std::cout << "UID: " << uid << std::endl;
    std::cout << "is_static:    " << getIsStatic() << std::endl;
    std::cout << "cv_qualifier: "
              << static_cast<std::underlying_type<CVQualifier>::type>(
                     getCVQualifier())
              << std::endl;
}

std::shared_ptr<ArrayType> ArrayType::init(std::shared_ptr<Type> _base_type,
                                           std::vector<size_t> _dims) {
    return init(std::move(_base_type), std::move(_dims),
                /* is static */ false, CVQualifier::NONE);
}

std::shared_ptr<ArrayType>
ArrayType::init(std::shared_ptr<Type> _base_type, std::vector<size_t> _dims,
                bool _is_static, CVQualifier _cv_qual, bool _is_uniform) {
    ArrayTypeKey key(_base_type, _dims, ArrayKind::MAX_ARRAY_KIND, _is_static,
                     _cv_qual, _is_uniform);
    auto find_res = array_type_set.find(key);
    if (find_res != array_type_set.end())
        return find_res->second;

    auto ret = std::make_shared<ArrayType>(_base_type, _dims, _is_static,
                                           _cv_qual, uid_counter++);
    ret->setIsUniform(_is_uniform);
    array_type_set[key] = ret;
    return ret;
}

std::string ArrayType::getName(std::shared_ptr<EmitCtx> ctx) {
    // TODO: we need a more correct way to do it
    return base_type->getName(ctx) + " *";
}

std::shared_ptr<ArrayType> ArrayType::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    IntTypeID base_type_id = rand_val_gen->getRandId(gen_pol->int_type_distr);
    auto base_type = IntegralType::init(base_type_id);

    // Determine how many dimensions do we want, relative to the current
    // ctx loop depth
    auto dims_use_kind = rand_val_gen->getRandId(gen_pol->array_dims_use_kind);
    size_t dims_num = ctx->generateNumberOfDims(dims_use_kind);
    assert(dims_num <= gen_pol->array_dims_num_limit &&
           "Arrays can't have more dimensions than the limit");
    std::vector<size_t> dims(dims_num, ctx->getDimensions().front());

    return init(base_type, dims);
}

std::shared_ptr<Type> ArrayType::makeVarying() {
    return init(getBaseType()->makeVarying(), getDimensions(), getIsStatic(),
                getCVQualifier(), false);
}
