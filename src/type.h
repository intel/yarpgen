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

#pragma once

#include <climits>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "hash.h"
#include "ir_value.h"
#include "options.h"

namespace yarpgen {

class PopulateCtx;
class EmitCtx;
class Iterator;

// Abstract class that serves as a common ancestor for all types.
// Here by "type" we mean a backend type, which has certain properties
// and may be instantiated as different language types
// (i.e. signed 32 bit integer, which may be int, or int32_t).
// For now we support only one language type per backend type, but
// that may change in future.
class Type {
  public:
    Type()
        : is_static(false), cv_qualifier(CVQualifier::NONE), is_uniform(true) {}
    Type(bool _is_static, CVQualifier _cv_qual)
        : is_static(_is_static), cv_qualifier(_cv_qual), is_uniform(true) {}
    virtual ~Type() = default;

    virtual std::string getName(std::shared_ptr<EmitCtx> ctx) = 0;
    virtual void dbgDump() = 0;

    virtual bool isIntType() { return false; }
    virtual bool isArrayType() { return false; }

    bool getIsStatic() { return is_static; }
    void setIsStatic(bool _is_static) { is_static = _is_static; }
    CVQualifier getCVQualifier() { return cv_qualifier; }
    void setCVQualifier(CVQualifier _cv_qual) { cv_qualifier = _cv_qual; }

    bool isUniform() { return is_uniform; }

    virtual std::shared_ptr<Type> makeVarying() = 0;

  protected:
    void setIsUniform(bool _is_uniform) { is_uniform = _is_uniform; }

  private:
    bool is_static;
    CVQualifier cv_qualifier;

    // ISPC
    // TODO: arrays themselves aren't uniform or varying
    bool is_uniform;

    // We don't store name here because for compound types it might be tricky to
    // get them.
};

// Base class for floating-point and integral types
class ArithmeticType : public Type {
  public:
    ArithmeticType() : Type() {}
    ArithmeticType(bool _is_static, CVQualifier _cv_qual)
        : Type(_is_static, _cv_qual) {}
    virtual std::string getLiteralSuffix() { return ""; };
};

class FPType : public ArithmeticType {
    // TODO: it is a stub for now
};

class IntegralType : public ArithmeticType {
  public:
    IntegralType() : ArithmeticType() {}
    IntegralType(bool _is_static, CVQualifier _cv_qual)
        : ArithmeticType(_is_static, _cv_qual) {}
    bool isIntType() final { return true; }
    virtual IntTypeID getIntTypeId() = 0;
    virtual size_t getBitSize() = 0;
    virtual bool getIsSigned() = 0;
    virtual IRValue getMin() = 0;
    virtual IRValue getMax() = 0;

    std::string getLiteralSuffix() override;

    // These utility functions take IntegerTypeID and return shared pointer to
    // corresponding type
    static std::shared_ptr<IntegralType> init(IntTypeID _type_id);
    static std::shared_ptr<IntegralType> init(IntTypeID _type_id,
                                              bool _is_static,
                                              CVQualifier _cv_qual,
                                              bool _is_uniform = true);

    static bool isSame(std::shared_ptr<IntegralType> &lhs,
                       std::shared_ptr<IntegralType> &rhs);

    // Auxiliary function for arithmetic conversions that shows if type a can
    // represent all the values of type b
    static bool canRepresentType(IntTypeID a, IntTypeID b);
    // Auxiliary function for arithmetic conversions that find corresponding
    // unsigned type
    static IntTypeID getCorrUnsigned(IntTypeID id);

    std::shared_ptr<Type> makeVarying() override;

  protected:
    // ISPC
    std::string getIspcNameHelper() {
        return (isUniform() ? "uniform" : "varying") + std::string(" ");
    }
    std::string getNameImpl(std::shared_ptr<EmitCtx> ctx, std::string raw_name);

  private:
    // There is a fixed small number of possible integral types,
    // so we use a folding set in order to save memory
    static std::unordered_map<IntTypeKey, std::shared_ptr<IntegralType>,
                              IntTypeKeyHasher>
        int_type_set;
};

template <typename T> class IntegralTypeHelper : public IntegralType {
  public:
    using value_type = T;
    IntegralTypeHelper(IntTypeID type_id, bool _is_static, CVQualifier _cv_qual)
        : IntegralType(_is_static, _cv_qual) {
        min = IRValue(type_id);
        min.getValueRef<T>() = std::numeric_limits<T>::min();
        min.setUBCode(UBKind::NoUB);

        max = IRValue(type_id);
        max.getValueRef<T>() = std::numeric_limits<T>::max();
        max.setUBCode(UBKind::NoUB);
    }

    size_t getBitSize() override { return sizeof(T) * CHAR_BIT; }
    bool getIsSigned() override { return std::is_signed<T>::value; }
    IRValue getMin() override { return min; }
    IRValue getMax() override { return max; }

  protected:
    IRValue min;
    IRValue max;
};

class TypeBool : public IntegralTypeHelper<bool> {
  public:
    TypeBool(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    //  TODO: decouple language representation of the type from back-end type.
    // I.e. for different languages the name of the type and the suffix might be
    // different.
    IntTypeID getIntTypeId() final { return IntTypeID::BOOL; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        Options &options = Options::getInstance();
        if (options.isC())
            return getNameImpl(ctx, "_Bool");
        return getNameImpl(ctx, "bool");
    }

    // For bool signedness is not defined, so std::is_signed and
    // std::is_unsigned return true. We treat them as unsigned
    bool getIsSigned() final { return false; }

    void dbgDump() final;
};

class TypeSChar : public IntegralTypeHelper<int8_t> {
  public:
    TypeSChar(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::SCHAR; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "signed char");
    }

    void dbgDump() final;
};

class TypeUChar : public IntegralTypeHelper<uint8_t> {
  public:
    TypeUChar(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::UCHAR; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "unsigned char");
    }

    void dbgDump() final;
};

class TypeSShort : public IntegralTypeHelper<int16_t> {
  public:
    TypeSShort(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::SHORT; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "short");
    }

    void dbgDump() final;
};

class TypeUShort : public IntegralTypeHelper<uint16_t> {
  public:
    TypeUShort(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::USHORT; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "unsigned short");
    }

    void dbgDump() final;
};

class TypeSInt : public IntegralTypeHelper<int32_t> {
  public:
    TypeSInt(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::INT; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "int");
    }

    void dbgDump() final;
};

class TypeUInt : public IntegralTypeHelper<uint32_t> {
  public:
    TypeUInt(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::UINT; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "unsigned int");
    }
    std::string getLiteralSuffix() final { return "U"; }

    void dbgDump() final;
};

class TypeSLLong : public IntegralTypeHelper<int64_t> {
  public:
    TypeSLLong(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::LLONG; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "long long int");
    }
    std::string getLiteralSuffix() final { return "LL"; }

    void dbgDump() final;
};

class TypeULLong : public IntegralTypeHelper<uint64_t> {
  public:
    TypeULLong(bool _is_static, CVQualifier _cv_qual)
        : IntegralTypeHelper(getIntTypeId(), _is_static, _cv_qual) {}

    IntTypeID getIntTypeId() final { return IntTypeID::ULLONG; }
    std::string getName(std::shared_ptr<EmitCtx> ctx) final {
        return getNameImpl(ctx, "unsigned long long int");
    }
    std::string getLiteralSuffix() final { return "ULL"; }

    void dbgDump() final;
};

// Base class for all of the array-like types (C-style, Vector, Array,
// ValArray).
class ArrayType : public Type {
  public:
    ArrayType(std::shared_ptr<Type> _base_type, std::vector<size_t> _dims,
              bool _is_static, CVQualifier _cv_qual, size_t _uid)
        : Type(_is_static, _cv_qual), base_type(std::move(_base_type)),
          dimensions(std::move(_dims)), kind(ArrayKind::MAX_ARRAY_KIND),
          uid(_uid) {}

    bool isArrayType() final { return true; }
    std::shared_ptr<Type> getBaseType() { return base_type; }
    std::vector<size_t> &getDimensions() { return dimensions; }
    size_t getUID() { return uid; }

    static bool isSame(const std::shared_ptr<ArrayType> &lhs,
                       const std::shared_ptr<ArrayType> &rhs);

    std::string getName(std::shared_ptr<EmitCtx> ctx) override;
    void dbgDump() override;

    static std::shared_ptr<ArrayType>
    init(std::shared_ptr<Type> _base_type, std::vector<size_t> _dims,
         bool _is_static, CVQualifier _cv_qual, bool _is_uniform = true);
    static std::shared_ptr<ArrayType> init(std::shared_ptr<Type> _base_type,
                                           std::vector<size_t> _dims);
    static std::shared_ptr<ArrayType> create(std::shared_ptr<PopulateCtx> ctx);

    std::shared_ptr<Type> makeVarying() override;

  private:
    // Folding set for all of the array types.
    static std::unordered_map<ArrayTypeKey, std::shared_ptr<ArrayType>,
                              ArrayTypeKeyHasher>
        array_type_set;
    // The easiest way to compare array types is to assign a unique identifier
    // to each of them and then compare it.
    static size_t uid_counter;

    std::shared_ptr<Type> base_type;
    // Number of elements in each dimension
    std::vector<size_t> dimensions;
    // The kind of array that we want to use during lowering process
    ArrayKind kind;
    size_t uid;
};

} // namespace yarpgen
