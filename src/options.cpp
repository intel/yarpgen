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

#include <algorithm>

#include "options.h"

using namespace yarpgen;

Options* yarpgen::options;

Options::Options() : standard_id(CXX11), mode_64bit(true),
                     include_valarray(false), include_vector(false), include_array(false) {
    plane_yarpgen_version = yarpgen_version;
    plane_yarpgen_version.erase(std::remove(plane_yarpgen_version.begin(), plane_yarpgen_version.end(), '.'),
                                plane_yarpgen_version.end());
}

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

bool Options::is_c() {
    return C99 <= standard_id && standard_id < MAX_CStandardID;
}

bool Options::is_cxx() {
    return CXX98 <= standard_id && standard_id < MAX_CXXStandardID;
}

bool Options::is_opencl() {
    return OpenCL_1_0 <= standard_id && standard_id < MAX_OpenCLStandardID;
}
