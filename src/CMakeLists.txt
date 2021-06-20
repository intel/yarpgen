###############################################################################
#
# Copyright (c) 2019-2020, Intel Corporation
# Copyright (c) 2019-2020, University of Utah
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
    "emit_policy.cpp"
    "emit_policy.h"
    "enums.h"
    "expr.cpp"
    "expr.h"
    "gen_policy.cpp"
    "gen_policy.h"
    "hash.cpp"
    "hash.h"
    "ir_node.h"
    "ir_value.cpp"
    "ir_value.h"
    "options.cpp"
    "options.h"
    "program.cpp"
    "program.h"
    "statistics.cpp"
    "statistics.h"
    "stmt.cpp"
    "stmt.h"
    "type.cpp"
    "type.h"
    "utils.cpp"
    "utils.h")

# Common std and build flags for all executables
set(STD cxx_std_17)
set(FLAGS #  $<$<CXX_COMPILER_ID:MSVC>:/WX>
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Wpedantic -Werror>
  -DBUILD_VERSION="${GIT_HASH}" -DBUILD_DATE="${BUILD_DATE}"
  -DYARPGEN_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}" -DYARPGEN_VERSION_MINOR="${PROJECT_VERSION_MINOR}")

# Static library to avoid building sources multiple times
add_library(yarpgen_lib STATIC ${LIB_SRCS})
target_compile_features(yarpgen_lib PRIVATE ${STD})
target_compile_options(yarpgen_lib PRIVATE ${FLAGS})

# Main executable
add_executable(yarpgen main.cpp)
target_compile_features(yarpgen PRIVATE ${STD})
target_compile_options(yarpgen PRIVATE ${FLAGS})
target_link_libraries(yarpgen yarpgen_lib)
# Copy main executable next to scripts for convenience
add_custom_command(TARGET yarpgen
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:yarpgen> ${CMAKE_SOURCE_DIR}/scripts)

# Test executables
add_executable(type_test type_test.cpp)
target_compile_features(type_test PRIVATE ${STD})
target_compile_options(type_test PRIVATE ${FLAGS})
target_link_libraries(type_test yarpgen_lib)

add_executable(data_test data_test.cpp)
target_compile_features(data_test PRIVATE ${STD})
target_compile_options(data_test PRIVATE ${FLAGS})
target_link_libraries(data_test yarpgen_lib)

add_executable(expr_test expr_test.cpp)
target_compile_features(expr_test PRIVATE ${STD})
target_compile_options(expr_test PRIVATE ${FLAGS})
target_link_libraries(expr_test yarpgen_lib)

add_executable(gen_test gen_test.cpp)
target_compile_features(gen_test PRIVATE ${STD})
target_compile_options(gen_test PRIVATE ${FLAGS})
target_link_libraries(gen_test yarpgen_lib)
