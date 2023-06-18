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

#include "context.h"
#include "data.h"

using namespace yarpgen;

static const size_t MAX_DIMS = 4;
static const size_t MAX_SIZE = 256;

// Will be used to obtain a seed for the random number engine
static std::random_device rd;
static std::mt19937 generator;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__            \
                      << ", function " << __func__ << "():\n    " << (msg)     \
                      << std::endl;                                            \
            abort();                                                           \
        }                                                                      \
    } while (false)

void scalarVarTest() {
    // Scalar Variable Test
    for (auto i = static_cast<int>(IntTypeID::BOOL);
         i < static_cast<int>(IntTypeID::MAX_INT_TYPE_ID); ++i)
        for (auto j = static_cast<int>(CVQualifier::NONE);
             j <= static_cast<int>(CVQualifier::CONST_VOLAT); ++j)
            for (int k = false; k <= true; ++k) {
                std::shared_ptr<IntegralType> ptr_to_type = IntegralType::init(
                    static_cast<IntTypeID>(i), static_cast<bool>(k),
                    static_cast<CVQualifier>(j));
                auto scalar_var = std::make_shared<ScalarVar>(
                    std::to_string(i), ptr_to_type, ptr_to_type->getMin());
                scalar_var->setCurrentValue(ptr_to_type->getMax());

                CHECK(scalar_var->getName(std::make_shared<EmitCtx>()) ==
                          std::to_string(i),
                      "Name");
                CHECK(scalar_var->getType() == ptr_to_type, "Type");
                CHECK(scalar_var->getUBCode() ==
                          ptr_to_type->getMin().getUBCode(),
                      "UB Code");
                CHECK(!scalar_var->hasUB(), "Has UB");

                CHECK(scalar_var->isScalarVar(), "ScalarVar identity");
                CHECK(scalar_var->getKind() == DataKind::VAR, "ScalarVar kind");

                // Comparison operations are defined not for all types, so we
                // need cast trick
                IRValue init_val =
                    scalar_var->getInitValue().castToType(IntTypeID::ULLONG);
                IRValue min_val =
                    ptr_to_type->getMin().castToType(IntTypeID::ULLONG);
                IRValue cur_val =
                    scalar_var->getCurrentValue().castToType(IntTypeID::ULLONG);
                IRValue max_val =
                    ptr_to_type->getMax().castToType(IntTypeID::ULLONG);
                CHECK((init_val == min_val).getValueRef<bool>(), "Init Value");
                CHECK((cur_val == max_val).getValueRef<bool>(), "CurrentValue");
                CHECK(scalar_var->getIsDead(), "Is dead");
            }
}

void arrayTest() {
    for (auto i = static_cast<int>(IntTypeID::BOOL);
         i < static_cast<int>(IntTypeID::MAX_INT_TYPE_ID); ++i)
        for (auto j = static_cast<int>(CVQualifier::NONE);
             j <= static_cast<int>(CVQualifier::CONST_VOLAT); ++j)
            for (int k = false; k <= true; ++k) {

                std::shared_ptr<IntegralType> ptr_to_type = IntegralType::init(
                    static_cast<IntTypeID>(i), static_cast<bool>(k),
                    static_cast<CVQualifier>(j));

                auto scalar_var = std::make_shared<ScalarVar>(
                    std::to_string(i), ptr_to_type, ptr_to_type->getMin());

                size_t dim_size = std::uniform_int_distribution<size_t>(
                    1, MAX_DIMS)(generator);
                std::vector<size_t> dims;
                dims.reserve(dim_size);
                for (size_t l = 0; l < dim_size; ++l)
                    dims.push_back(std::uniform_int_distribution<size_t>(
                        1, MAX_SIZE)(generator));
                auto array_type =
                    ArrayType::init(ptr_to_type, dims, static_cast<bool>(k),
                                    static_cast<CVQualifier>(k));

                auto array = std::make_shared<Array>(
                    std::to_string(i), array_type, ptr_to_type->getMin());

                CHECK(array->getName(std::make_shared<EmitCtx>()) ==
                          std::to_string(i),
                      "Name");
                CHECK(array->getType() == array_type, "Type");
                CHECK(array->getUBCode() == ptr_to_type->getMin().getUBCode(),
                      "UB Code");
                CHECK(!array->hasUB(), "Has UB");

                CHECK(array->isArray(), "Array identity");
                CHECK(array->getKind() == DataKind::ARR, "Array kind");

                array->setCurrentValue(ptr_to_type->getMax(),
                                       Options::main_val_idx);

                CHECK(
                    (array->getInitValues(Options::main_val_idx)
                         .getAbsValue() == ptr_to_type->getMin().getAbsValue()),
                    "Init Value");
                CHECK(
                    (array->getCurrentValues(Options::main_val_idx)
                         .getAbsValue() == ptr_to_type->getMax().getAbsValue()),
                    "CurrentValue");
                CHECK(array->getIsDead(), "Is dead");
            }
}

int main() {
    auto seed = rd();
    std::cout << "Test seed: " << seed << std::endl;
    generator.seed(seed);

    scalarVarTest();
    arrayTest();
    // TODO: we need an iterator test, but it is better to add it after
    // expressions test
}
