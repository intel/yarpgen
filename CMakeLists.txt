###############################################################################
#
# Copyright (c) 2018-2020, Intel Corporation
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

cmake_minimum_required(VERSION 3.6)
project(YARPGen
        VERSION 2.0
        LANGUAGES CXX)

STRING(TIMESTAMP BUILD_DATE "%Y:%m:%d")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Build type not specified: Use Release by default.")
endif(NOT CMAKE_BUILD_TYPE)

find_package(Git)
set(GIT_HASH "no_version_info")
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --format=%h HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(src)
