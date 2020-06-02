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

#include <iostream>
#include <memory>
#include <vector>

#include "enums.h"

namespace yarpgen {

const size_t HASH_SEED = 42;

// Class that is used to calculate various hashes.
// Right now it is used for folding sets of various types.
class Hash {
  public:
    Hash() : seed(HASH_SEED) {}

    // TODO: we have a collisions. We need to think about better hash-functions
    template <typename T>
    inline typename std::enable_if<std::is_fundamental<T>::value, void>::type
    operator()(T value) {
        hashCombine(std::hash<T>()(value));
    }

    // TODO we don't need separate template for enums, because C++ 14 allows to
    // pass them to std::hash. The issue here is that Ubuntu 16.04 doesn't
    // provide a library that supports that. When we switch to Ubuntu 18.04, we
    // can get rid of this function.
    template <typename T>
    inline typename std::enable_if<!std::is_fundamental<T>::value, void>::type
    operator()(T value) {
        static_assert(
            std::is_enum<T>::value,
            "In current implementation we need to hash only enum type");
        using enum_under_type = typename std::underlying_type<T>::type;
        hashCombine(
            std::hash<enum_under_type>()(static_cast<enum_under_type>(value)));
    }

    template <typename T> inline void operator()(std::vector<T> value) {
        Hash hash;
        for (const auto &elem : value)
            hash(elem);
        hashCombine(hash.getSeed());
    }

    size_t getSeed() { return seed; }

  private:
    // Combine existing seed with a new hash value.
    void hashCombine(size_t value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    size_t seed;
};

// We need a folding set for integral types. There is a fixed number of them.
// Otherwise we will just waste too much memory, because almost every object in
// IR has a type. This class is used as a key in the folding set.
class IntegralType;

class IntTypeKey {
  public:
    IntTypeKey(IntTypeID _int_type_id, bool _is_static,
               CVQualifier _cv_qualifier, bool _is_uniform);
    explicit IntTypeKey(std::shared_ptr<IntegralType> &item);
    bool operator==(const IntTypeKey &other) const;

    IntTypeID int_type_id;
    bool is_static;
    CVQualifier cv_qualifier;
    bool is_uniform;
};

// This class provides a hashing mechanism for folding set.
class IntTypeKeyHasher {
  public:
    std::size_t operator()(const IntTypeKey &key) const;
};

class Type;

// This class is used as a key in the folding set.
class ArrayTypeKey {
  public:
    ArrayTypeKey(std::shared_ptr<Type> _base_type, std::vector<size_t> _dims,
                 ArrayKind _kind, bool _is_static, CVQualifier _cv_qual,
                 bool _is_uniform);
    bool operator==(const ArrayTypeKey &other) const;

    std::shared_ptr<Type> base_type;
    std::vector<size_t> dims;
    ArrayKind kind;
    bool is_static;
    CVQualifier cv_qualifier;
    bool is_uniform;
};

// This class provides a hashing mechanism for folding set.
class ArrayTypeKeyHasher {
  public:
    std::size_t operator()(const ArrayTypeKey &key) const;
};
} // namespace yarpgen
