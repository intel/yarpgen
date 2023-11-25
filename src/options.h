/*
Copyright (c) 2017-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

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
#include <cstdint>
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

    static std::vector<OptionDescr> options_set;

  private:
    static void printVersion(std::string arg);
    static void printHelpAndExit(std::string error_msg = "");
    static bool optionStartsWith(char *option, const char *test);
    static bool parseShortArg(size_t argc, size_t &argv_iter, char **&argv,
                              OptionDescr &option);
    static bool parseLongArg(size_t &argv_iter, char **&argv,
                             OptionDescr &option);
    static bool parseLongAndShortArgs(size_t argc, size_t &argv_iter,
                                      char **&argv, OptionDescr &option);

    static void parseSeed(std::string seed_str);
    static void parseStandard(std::string std);
    static void parseCheckAlgo(std::string val);
    static void parseInpAsArgs(std::string val);
    static void parseEmitAlignAttr(std::string val);
    static void parseUniqueAlignSize(std::string val);
    static void parseAlignSize(std::string val);
    static void parseAllowDeadData(std::string val);
    static void parseEmitPragmas(std::string val);
    static void parseOutDir(std::string val);
    static void parseUseParamShuffle(std::string val);
    static void parseExplLoopParams(std::string val);
    static void parseMutationKind(std::string mutate_str);
    static void parseMutationSeed(std::string mutation_seed_str);
    static void parseAllowUBInDC(std::string allow_ub_in_dc_str);
};

class Options {
  public:
    // The number of divergent values that we support in loops
    // In theory, it can be made higher, but we assume that 2 is enough
    // for now. The agreement that main values are tied to the 0-th index
    static size_t constexpr vals_number = 2;
    static size_t constexpr main_val_idx = 0;
    static size_t constexpr alt_val_idx = 1;

    static Options &getInstance() {
        static Options instance;
        return instance;
    }
    Options(const Options &options) = delete;
    Options &operator=(const Options &) = delete;

    void setRawOptions(size_t argc, char *argv[]);

    void setSeed(uint64_t _seed) { seed = _seed; }
    uint64_t getSeed() { return seed; }

    void setLangStd(LangStd _std) { std = _std; }
    LangStd getLangStd() { return std; }
    bool isC() { return std == LangStd::C; }
    bool isCXX() { return std == LangStd::CXX; }
    bool isISPC() { return std == LangStd::ISPC; }
    bool isSYCL() { return std == LangStd::SYCL; }

    void setCheckAlgo(CheckAlgo val) { check_algo = val; }
    CheckAlgo getCheckAlgo() { return check_algo; }

    void setInpAsArgs(OptionLevel val) { inp_as_args = val; }
    OptionLevel inpAsArgs() { return inp_as_args; }

    void setEmitAlignAttr(OptionLevel _val) { emit_align_attr = _val; }
    OptionLevel getEmitAlignAttr() { return emit_align_attr; }

    void setUniqueAlignSize(bool _val) { unique_align_size = _val; }
    bool getUniqueAlignSize() { return unique_align_size; }

    void setAlignSize(AlignmentSize _val) { align_size = _val; }
    AlignmentSize getAlignSize() { return align_size; }

    void setAllowDeadData(bool val) { allow_dead_data = val; }
    bool getAllowDeadData() { return allow_dead_data; }

    void setEmitPragmas(OptionLevel _val) { emit_pragmas = _val; }
    OptionLevel getEmitPragmas() { return emit_pragmas; }

    void setOutDir(std::string _out_dir) { out_dir = _out_dir; }
    std::string getOutDir() { return out_dir; }

    void setUseParamShuffle(bool val) { use_param_shuffle = val; }
    bool getUseParamShuffle() { return use_param_shuffle; }

    void setExplLoopParams(bool val) { expl_loop_params = val; }
    bool getExplLoopParams() { return expl_loop_params; }

    void setMutationKind(MutationKind val) { mutation_kind = val; }
    MutationKind getMutationKind() { return mutation_kind; }

    void setMutationSeed(uint64_t val) { mutation_seed = val; }
    uint64_t getMutationSeed() { return mutation_seed; }

    void setAllowUBInDC(OptionLevel _val) { allow_ub_in_dc = _val; }
    OptionLevel getAllowUBInDC() { return allow_ub_in_dc; }

    void dump(std::ostream &stream);

  private:
    Options()
        : seed(0), std(LangStd::CXX), check_algo(CheckAlgo::HASH),
          inp_as_args(OptionLevel::SOME), emit_align_attr(OptionLevel::SOME),
          unique_align_size(false),
          align_size(AlignmentSize::MAX_ALIGNMENT_SIZE), allow_dead_data(false),
          emit_pragmas(OptionLevel::SOME), out_dir("."),
          use_param_shuffle(false), expl_loop_params(false),
          mutation_kind(MutationKind::NONE), mutation_seed(0),
          allow_ub_in_dc(OptionLevel::NONE) {}

    std::vector<std::string> raw_options;

    uint64_t seed;
    LangStd std;
    CheckAlgo check_algo;
    // Pass input data to a test function as parameters
    OptionLevel inp_as_args;

    // Options for alignment attributes
    OptionLevel emit_align_attr;
    bool unique_align_size;
    AlignmentSize align_size;

    bool allow_dead_data;

    OptionLevel emit_pragmas;

    std::string out_dir;

    bool use_param_shuffle;

    // Explicit loop parameters. Some applications need that option available
    bool expl_loop_params;

    MutationKind mutation_kind;
    uint64_t mutation_seed;

    // If we want to allow Undefined Behavior in Dead Code
    OptionLevel allow_ub_in_dc;
};
} // namespace yarpgen
