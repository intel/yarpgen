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
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace yarpgen {

class OptionDescr {
  public:
    OptionDescr(OptionKind _kind, std::string _short_arg, std::string _long_arg,
                bool _has_value, std::string _help_msg, std::string _err_msg,
                std::function<void(std::string)> _action, std::string _def_val,
                std::vector<std::string> _values)
        : kind(_kind), short_arg(std::move(_short_arg)),
          long_arg(std::move(_long_arg)), has_value(_has_value),
          help_msg(std::move(_help_msg)), err_msg(std::move(_err_msg)),
          action(std::move(_action)), def_val(std::move(_def_val)),
          values(std::move(_values)) {}
    OptionKind getKind() { return kind; }
    std::string getShortArg() { return short_arg; }
    std::string getLongArg() { return long_arg; }
    bool hasValue() { return has_value; }
    std::string getHelpMsg() { return help_msg; }
    std::string getErrMsg() { return err_msg; }
    std::function<void(std::string)> getAction() { return action; }
    std::string getDefaultVal() { return def_val; }
    std::vector<std::string> getAvailVals() { return values; }

  private:
    OptionKind kind;
    std::string short_arg;
    std::string long_arg;
    bool has_value;
    std::string help_msg;
    std::string err_msg;
    std::function<void(std::string)> action;
    std::string def_val;
    std::vector<std::string> values;
};

class Options;

class OptionParser {
  public:
    static void parse(size_t argc, char *argv[]);
    // Initialize options with default values
    static void initOptions();

  private:
    static void printVersion(std::string arg);
    static void printHelpAndExit(std::string error_msg = "");
    static bool optionStartsWith(char *option, const char *test);
    static bool parseShortArg(size_t argc, size_t &argv_iter, char **&argv,
                              OptionDescr option);
    static bool parseLongArg(size_t &argv_iter, char **&argv,
                             OptionDescr option);
    static bool parseLongAndShortArgs(int argc, size_t &argv_iter, char **&argv,
                                      OptionDescr option);

    static void parseSeed(std::string seed_str);
    static void parseStandard(std::string std);
    static void parseAsserts(std::string val);
    static void parseInpAsArgs(std::string val);

    static std::vector<OptionDescr> options_set;
};

class Options {
  public:
    static Options &getInstance() {
        static Options instance;
        return instance;
    }
    Options(const Options &options) = delete;
    Options &operator=(const Options &) = delete;

    void setSeed(size_t _seed) { seed = _seed; }
    size_t getSeed() { return seed; }

    void setLangStd(LangStd _std) { std = _std; }
    LangStd getLangStd() { return std; }
    bool isCXX() { return std == LangStd::CXX; }
    bool isISPC() { return std == LangStd::ISPC; }
    bool isSYCL() { return std == LangStd::SYCL; }

    void setUseAsserts(OptionLevel val) { use_asserts = val; }
    OptionLevel useAsserts() { return use_asserts; }

    void setInpAsArgs(OptionLevel val) { inp_as_args = val; }
    OptionLevel inpAsArgs() { return inp_as_args; }

  private:
    Options() : seed(0), std(LangStd::CXX), use_asserts(OptionLevel::SOME),
                inp_as_args(OptionLevel::SOME) {}

    size_t seed;
    LangStd std;
    OptionLevel use_asserts;
    // Pass input data to a test function as parameters
    OptionLevel inp_as_args;
};
} // namespace yarpgen
