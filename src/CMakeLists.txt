###############################################################################
#
# Copyright (c) 2018, Intel Corporation
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

set(LIB_SRCS type.cpp variable.cpp expr.cpp stmt.cpp gen_policy.cpp sym_table.cpp program.cpp options.cpp)

set(SRCS ${LIB_SRCS} main.cpp self-test.cpp)

add_executable(yarpgen ${SRCS})

target_compile_features(yarpgen PRIVATE cxx_std_14)
target_compile_definitions(yarpgen PRIVATE BUILD_VERSION="${GIT_HASH}" BUILD_DATE="${BUILD_DATE}")
target_compile_options(yarpgen PRIVATE
  #  $<$<CXX_COMPILER_ID:MSVC>:/WX>
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Wpedantic -Werror>)
