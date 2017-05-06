/*
Copyright (c) 2017, Intel Corporation

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

#include "options.h"

using namespace yarpgen;

Options* yarpgen::options;

const std::map<std::string, Options::StandardID> Options::str_to_standard = {
    {"c99", C99},
    {"c11", C11},

    {"c++98", CXX98},
    {"c++03", CXX03},
    {"c++11", CXX11},
    {"c++14", CXX14},
    {"c++17", CXX17},

    {"opencl_1_0", OpenCL_1_0},
    {"opencl_1_1", OpenCL_1_1},
    {"opencl_1_2", OpenCL_1_2},
    {"opencl_2_0", OpenCL_2_0},
    {"opencl_2_1", OpenCL_2_1},
    {"opencl_2_2", OpenCL_2_2},
};