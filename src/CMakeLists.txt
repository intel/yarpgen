###############################################################################
#
# Copyright (c) 2019, Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################

set(LIB_SRCS
    "context.cpp"
    "context.h"
    "data.cpp"
    "data.h"
    "enums.h"
    "expr.cpp"
    "expr.h"
    "hash.cpp"
    "hash.h"
    "ir_node.h"
    "ir_value.cpp"
    "stmt.cpp"
    "stmt.h"
    "type.cpp"
    "utils.cpp"
)


set(SRCS ${LIB_SRCS} main.cpp)

add_executable(yarpgen ${SRCS})

set(STD cxx_std_14)
set(FLAGS #  $<$<CXX_COMPILER_ID:MSVC>:/WX>
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Wpedantic -Werror>)

target_compile_features(yarpgen PRIVATE ${STD})
target_compile_definitions(yarpgen PRIVATE BUILD_VERSION="${GIT_HASH}" BUILD_DATE="${BUILD_DATE}"
    YARPGEN_VERSION_MAJOR="${YARPGEN_VERSION_MAJOR}" YARPGEN_VERSION_MINOR="${YARPGEN_VERSION_MINOR}")
target_compile_options(yarpgen PRIVATE ${FLAGS})

add_executable(type_test ${LIB_SRCS} type_test.cpp)
target_compile_features(type_test PRIVATE ${STD})
target_compile_options(type_test PRIVATE ${FLAGS})

add_executable(data_test ${LIB_SRCS} data_test.cpp)
target_compile_features(data_test PRIVATE ${STD})
target_compile_options(data_test PRIVATE ${FLAGS})

add_executable(expr_test ${LIB_SRCS} expr_test.cpp)
target_compile_features(expr_test PRIVATE ${STD})
target_compile_options(expr_test PRIVATE ${FLAGS})
