/*
Copyright (c) 2020, Intel Corporation
Copyright (c) 2020, University of Utah

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

#include "enums.h"
#include "utils.h"

using namespace yarpgen;

std::string yarpgen::toString(OptionLevel val) {
    switch (val) {
        case OptionLevel::NONE:
            return "none";
        case OptionLevel::SOME:
            return "some";
        case OptionLevel::ALL:
            return "all";
        case OptionLevel::MAX_OPTION_LEVEL:
            ERROR("Bad option level");
    }
    ERROR("Unreachable");
}

std::string yarpgen::toString(LangStd val) {
    switch (val) {
        case LangStd::CXX:
            return "c++";
        case LangStd::ISPC:
            return "ispc";
        case LangStd::SYCL:
            return "sycl";
        case LangStd::MAX_LANG_STD:
            ERROR("Bad lang std");
    }
    ERROR("Unreachable");
}

std::string yarpgen::toString(AlignmentSize val) {
    switch (val) {
        case AlignmentSize::A16:
            return "16";
        case AlignmentSize::A32:
            return "32";
        case AlignmentSize::A64:
            return "64";
        case AlignmentSize::MAX_ALIGNMENT_SIZE:
            return "rand";
    }
    ERROR("Unreachable");
}
