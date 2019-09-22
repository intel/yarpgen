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

set(LIB_SRCS type.cpp ir_value.cpp enums.h)

set(SRCS ${LIB_SRCS} main.cpp)

add_executable(yarpgen ${SRCS})

target_compile_features(yarpgen PRIVATE cxx_std_14)
target_compile_definitions(yarpgen PRIVATE BUILD_VERSION="${GIT_HASH}" BUILD_DATE="${BUILD_DATE}"
    YARPGEN_VERSION_MAJOR="${YARPGEN_VERSION_MAJOR}" YARPGEN_VERSION_MINOR="${YARPGEN_VERSION_MINOR}")
target_compile_options(yarpgen PRIVATE
  #  $<$<CXX_COMPILER_ID:MSVC>:/WX>
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Wpedantic -Werror>)