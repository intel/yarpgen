/*
Copyright (c) 2017-2020, Intel Corporation

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
#pragma once

#include "enums.h"
#include <cstdlib>
#include <functional>
#include <map>
#include <tuple>
#include <vector>

namespace yarpgen {
class OptionParser {
  public:
    static void parse(size_t argc, char *argv[]);

  private:
    using option_t = std::tuple<std::string, std::string, bool, std::string,
                                std::string, std::function<void(std::string)>>;

    static void printVersion(std::string arg);
    static void printHelpAndExit(std::string error_msg = "");
    static bool optionStartsWith(char *option, const char *test);
    static bool parseShortArg(size_t argc, size_t &argv_iter, char **&argv,
                              option_t option);
    static bool parseLongArg(size_t &argv_iter, char **&argv, option_t option);
    static bool parseLongAndShortArgs(int argc, size_t &argv_iter, char **&argv,
                                      option_t option);

    static void parseSeed(std::string seed_str);
    static void parseStandard(std::string std);

    // Short argument, long argument, has_value, help message, error message,
    // action function
    static std::vector<option_t> opt_codes;
};

class Options {
  public:
    static Options &getInstance() {
        static Options instance;
        return instance;
    }
    Options(const Options &root) = delete;
    Options &operator=(const Options &) = delete;

    void setSeed(size_t _seed) { seed = _seed; }
    size_t getSeed() { return seed; }

    void setLangStd(LangStd _std) { std = _std; }
    LangStd getLangStd() { return std; }

  private:
    Options() : seed(0), std(LangStd::CXX) {}

    size_t seed;
    LangStd std;
};
} // namespace yarpgen
