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

#include <functional>
#include <memory>
#include <type_traits>

#include "enums.h"
#include "hash.h"
#include "type.h"
#include "utils.h"

using namespace yarpgen;

IntTypeKey::IntTypeKey(IntTypeID _int_type_id, bool _is_static,
                       CVQualifier _cv_qualifier, bool _is_uniform)
    : int_type_id(_int_type_id), is_static(_is_static),
      cv_qualifier(_cv_qualifier), is_uniform(_is_uniform) {}

IntTypeKey::IntTypeKey(std::shared_ptr<IntegralType> &item) {
    int_type_id = item->getIntTypeId();
    is_static = item->getIsStatic();
    cv_qualifier = item->getCVQualifier();
    is_uniform = item->isUniform();
}

bool IntTypeKey::operator==(const IntTypeKey &other) const {
    return (int_type_id == other.int_type_id) &&
           (is_static == other.is_static) &&
           (cv_qualifier == other.cv_qualifier) &&
           (is_uniform == other.is_uniform);
}

std::size_t IntTypeKeyHasher::operator()(const IntTypeKey &key) const {
    Hash hash;
    hash(key.int_type_id);
    hash(key.is_static);
    hash(key.cv_qualifier);
    hash(key.is_uniform);
    return hash.getSeed();
}

ArrayTypeKey::ArrayTypeKey(std::shared_ptr<Type> _base_type,
                           std::vector<size_t> _dims, ArrayKind _kind,
                           bool _is_static, CVQualifier _cv_qual,
                           bool _is_uniform)
    : base_type(std::move(_base_type)), dims(std::move(_dims)), kind(_kind),
      is_static(_is_static), cv_qualifier(_cv_qual), is_uniform(_is_uniform) {}

bool ArrayTypeKey::operator==(const ArrayTypeKey &other) const {
    if (base_type != other.base_type)
        return false;

    if (base_type->isIntType()) {
        auto base_int_type = std::static_pointer_cast<IntegralType>(base_type);
        auto other_int_type =
            std::static_pointer_cast<IntegralType>(other.base_type);
        if (!IntegralType::isSame(base_int_type, other_int_type)) {
            return false;
        }
    }
    else {
        ERROR("Unsupported base type for array!");
    }

    return (dims == other.dims) && (kind == other.kind) &&
           (is_static == other.is_static) &&
           (cv_qualifier == other.cv_qualifier) &&
           (is_uniform == other.is_uniform);
}

std::size_t ArrayTypeKeyHasher::operator()(const ArrayTypeKey &key) const {
    Hash hash;

    if (key.base_type->isIntType()) {
        auto base_int_type =
            std::static_pointer_cast<IntegralType>(key.base_type);
        IntTypeKeyHasher base_int_hasher;
        size_t base_int_hash = base_int_hasher(IntTypeKey(base_int_type));
        hash(base_int_hash);
    }
    else {
        ERROR("Unsupported base type for array");
    }

    hash(key.dims);
    hash(key.kind);
    hash(key.is_static);
    hash(key.cv_qualifier);
    hash(key.is_uniform);

    return hash.getSeed();
}
