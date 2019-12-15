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

#include "data.h"
#include "expr.h"

using namespace yarpgen;

int main() {
    IRValue start_val (IntTypeID::INT);
    start_val.getValueRef<int32_t>() = 0;
    start_val.setIsUndefined(false);
    auto start_expr = std::make_shared<ConstantExpr>(start_val);

   IRValue end_val (IntTypeID::INT);
   end_val.getValueRef<int32_t>() = 0;
   end_val.setIsUndefined(false);
   auto end_expr = std::make_shared<ConstantExpr>(end_val);


}
