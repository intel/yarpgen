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

#include "options.h"
#include "utils.h"
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>

using namespace yarpgen;

#ifndef YARPGEN_VERSION_MAJOR
#define YARPGEN_VERSION_MAJOR "0"
#endif

#ifndef YARPGEN_VERSION_MINOR
#define YARPGEN_VERSION_MINOR "0"
#endif

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION ""
#endif

using namespace yarpgen;

static const size_t PADDING = 30;

// Short argument, long argument, has_value, help message, error message,
// action function, default, possible values
std::vector<OptionDescr> yarpgen::OptionParser::options_set{
    {OptionKind::HELP,
     "-h",
     "--help",
     false,
     "Display help message",
     "Unreachable Error",
     OptionParser::printHelpAndExit,
     "",
     {}},
    {OptionKind::VERSION,
     "-v",
     "--version",
     false,
     "Print YARPGen version",
     "Unreachable Error",
     OptionParser::printVersion,
     "",
     {}},
    {OptionKind::SEED,
     "-s",
     "--seed",
     true,
     "Pass a predefined seed (0 is reserved for random)",
     "Unreachable Error",
     OptionParser::parseSeed,
     "0",
     {}},
    {OptionKind::STD,
     "",
     "--std",
     true,
     "Language standard of the test",
     "Can't recognize standard",
     OptionParser::parseStandard,
     "c++",
     {"c", "c++", "ispc", "sycl"}},
    {OptionKind::CHECK_ALGO,
     "",
     "--check-algo",
     true,
     "What check algorithm to use",
     "Can't parse check algo",
     OptionParser::parseCheckAlgo,
     "hash",
     {"hash", "asserts"}},
    {OptionKind::INP_AS_ARGS,
     "",
     "--inp-as-args",
     true,
     "Pass input data as a parameters",
     "Can't parse input as args",
     OptionParser::parseInpAsArgs,
     "some",
     {"none", "some", "all"}},
    {OptionKind::EMIT_ALIGN_ATTR,
     "",
     "--emit-align-attr",
     true,
     "Emit \"aligned\" attributes for arrays",
     "Can't parse emit aligned attributes",
     OptionParser::parseEmitAlignAttr,
     "some",
     {"none", "some", "all"}},
    {OptionKind::UNIQUE_ALIGN_SIZE,
     "",
     "--unique-align-size",
     false,
     "Use unique align size for all attributes",
     "Can't parse emit unique align size",
     OptionParser::parseUniqueAlignSize,
     "false",
     {"true", "false"}},
    {OptionKind::ALIGN_SIZE,
     "",
     "--align-size",
     true,
     "Size for \"aligned\" attributes for arrays",
     "Can't parse alignment size",
     OptionParser::parseAlignSize,
     "rand",
     {"16", "32", "64"}},
    {OptionKind::ALLOW_DEAD_DATA,
     "",
     "--allow-dead-data",
     true,
     "Allow to create data(vars and arrays) that will be never used",
     "Can't parse allow dead data",
     OptionParser::parseAllowDeadData,
     "false",
     {"true", "false"}},
    {OptionKind::EMIT_PRAGMAS,
     "",
     "--emit-pragmas",
     true,
     "Emit pragmas",
     "Can't parse emit pragmas",
     OptionParser::parseEmitPragmas,
     "some",
     {"none", "some", "all"}},
    {OptionKind::OUT_DIR,
     "-o",
     "--out-dir",
     true,
     "Folder for generated test files (it should exist)",
     "Unreachable Error",
     OptionParser::parseOutDir,
     ".",
     {}},
    {OptionKind::PARAM_SHUFFLE,
     "",
     "--param-shuffle",
     true,
     "Use parameters shuffling",
     "Can't parse use parameters shuffling",
     OptionParser::parseUseParamShuffle,
     "true",
     {"true", "false"}},
    {OptionKind::EXPL_LOOP_PARAM,
     "",
     "--expl-loop-param",
     true,
     "Emit explicit loop's parameters",
     "Can't parse use explicit loop parameters",
     OptionParser::parseExplLoopParams,
     "false",
     {"true", "false"}},
    {OptionKind::MUTATE,
     "",
     "--mutate",
     true,
     "Test mutation type. Can be disabled, set to mutate expressions "
     "only or mutate mote parameters",
     "Can't parse mutate parameter",
     OptionParser::parseMutationKind,
     "none",
     {"none", "exprs", "all"}},
    {OptionKind::MUTATION_SEED,
     "",
     "--mutation-seed",
     true,
     "Pass a predefined mutation seed (0 is reserved for random)",
     "Unreachable Error",
     OptionParser::parseMutationSeed,
     "0",
     {}},
    {OptionKind::UB_IN_DC,
     "",
     "--allow-ub-in-dc",
     true,
     "Whether we allow Undefined Behavior in Dead Code ",
     "Can't parse AllowUBinDC",
     OptionParser::parseAllowUBInDC,
     "none",
     {"none", "some", "all"}},
};

static void dumpVersion(std::ostream &stream) {
    stream << "yarpgen version " << YARPGEN_VERSION_MAJOR << "."
           << YARPGEN_VERSION_MINOR << " (build " << BUILD_VERSION << " on "
           << BUILD_DATE << ")" << std::endl;
}

void OptionParser::printVersion(std::string arg) {
    dumpVersion(std::cout);
    if (!arg.empty())
        exit(-1);
    exit(0);
}

void OptionParser::printHelpAndExit(std::string error_msg) {
    if (!error_msg.empty())
        std::cerr << error_msg << std::endl;

    std::cout << "Usage: yarpgen " << std::endl;

    auto print_helper = [](std::string item, bool value = false,
                           bool sep = true, int num_printed = -1) -> size_t {
        if (item.empty())
            return 0;
        if (num_printed >= 0)
            std::cout << std::string(PADDING - num_printed, ' ');
        std::string output =
            item + (value ? "=<value>" : "") + (sep ? ", " : "");
        std::cout << output;
        return output.size();
    };

    for (auto &item : options_set) {
        size_t num_printed = 0;
        std::cout << "    ";
        num_printed += print_helper(item.getShortArg());
        num_printed += print_helper(item.getLongArg(), item.hasValue(), false);
        print_helper(item.getHelpMsg(), false, false,
                     static_cast<int>(num_printed));
        if (!item.getDefaultVal().empty())
            std::cout << " (Default: " << item.getDefaultVal() << ")";
        if (!item.getAvailVals().empty()) {
            std::cout << std::endl;
            std::stringstream ss;
            ss << "    Possible values: ";
            for (const auto &avail_val : item.getAvailVals()) {
                ss << avail_val
                   << (avail_val != item.getAvailVals().back() ? ", " : "");
            }
            print_helper(ss.str(), false, false, 0);
        }
        std::cout << std::endl;
    }

    printVersion(error_msg);
}

bool OptionParser::optionStartsWith(char *option, const char *test) {
    return !strncmp(option, test, strlen(test));
}

// This function handles command-line options in form of "-short_arg <value>"
// and performs action(<value>)
bool OptionParser::parseShortArg(size_t argc, size_t &argv_iter, char **&argv,
                                 OptionDescr &option) {
    std::string short_arg = option.getShortArg();
    bool has_value = option.hasValue();
    auto action = option.getAction();
    if (!strcmp(argv[argv_iter], short_arg.c_str())) {
        if (has_value)
            argv_iter++;
        if (argv_iter == argc)
            printHelpAndExit(option.getErrMsg());
        else {
            if (has_value)
                action(argv[argv_iter]);
            else
                action("");
            return true;
        }
    }
    return false;
}

// This function handles command-line options in form of "--long_arg=<value>"
// and performs action(<value>)
bool OptionParser::parseLongArg(size_t &argv_iter, char **&argv,
                                OptionDescr &option) {
    std::string long_arg = option.getLongArg();
    bool has_value = option.hasValue();
    auto action = option.getAction();
    if (has_value)
        long_arg = long_arg + "=";
    if (optionStartsWith(argv[argv_iter], long_arg.c_str())) {
        if (has_value) {
            if (strlen(argv[argv_iter]) == long_arg.size())
                printHelpAndExit(option.getErrMsg());
            else {
                action(argv[argv_iter] + long_arg.size());
                return true;
            }
        }
        else {
            if (strlen(argv[argv_iter]) == long_arg.size()) {
                action("");
                return true;
            }
            else
                printHelpAndExit(option.getErrMsg());
        }
    }
    return false;
}

bool OptionParser::parseLongAndShortArgs(size_t argc, size_t &argv_iter,
                                         char **&argv, OptionDescr &option) {
    return parseLongArg(argv_iter, argv, option) ||
           parseShortArg(argc, argv_iter, argv, option);
}

void OptionParser::parse(size_t argc, char *argv[]) {
    Options &options = Options::getInstance();
    options.setRawOptions(argc, argv);

    for (size_t i = 1; i < argc; ++i) {
        bool parsed = false;
        for (auto &item : options_set)
            if (parseLongAndShortArgs(argc, i, argv, item)) {
                parsed = true;
                break;
            }
        if (!parsed)
            printHelpAndExit("Unknown option: " + std::string(argv[i]));
    }
}

void OptionParser::initOptions() {
    for (auto &item : options_set) {
        OptionKind kind = item.getKind();
        std::string def_val = item.getDefaultVal();

        if (kind == OptionKind::HELP || kind == OptionKind::VERSION)
            continue;
        auto action = item.getAction();
        action(def_val);
    }
}

void OptionParser::parseSeed(std::string seed_str) {
    std::stringstream arg_ss(seed_str);
    Options &options = Options::getInstance();
    uint64_t seed = 0;
    arg_ss >> seed;
    options.setSeed(seed);
}

void OptionParser::parseStandard(std::string std) {
    Options &options = Options::getInstance();
    if (std == "c")
        options.setLangStd(LangStd::C);
    else if (std == "c++")
        options.setLangStd(LangStd::CXX);
    else if (std == "ispc")
        options.setLangStd(LangStd::ISPC);
    else if (std == "sycl")
        options.setLangStd(LangStd::SYCL);
    else
        printHelpAndExit("Bad language standard");
}

void OptionParser::parseCheckAlgo(std::string val) {
    Options &options = Options::getInstance();
    if (val == "hash")
        options.setCheckAlgo(CheckAlgo::HASH);
    else if (val == "asserts")
        options.setCheckAlgo(CheckAlgo::ASSERTS);
    else
        printHelpAndExit("Can't recognize checking algorithm");
}

void OptionParser::parseInpAsArgs(std::string val) {
    Options &options = Options::getInstance();
    if (val == "none")
        options.setInpAsArgs(OptionLevel::NONE);
    else if (val == "some")
        options.setInpAsArgs(OptionLevel::SOME);
    else if (val == "all")
        options.setInpAsArgs(OptionLevel::ALL);
    else
        printHelpAndExit("Can't recognize input as arguments use level");
}

void OptionParser::parseEmitAlignAttr(std::string val) {
    Options &options = Options::getInstance();
    if (val == "none")
        options.setEmitAlignAttr(OptionLevel::NONE);
    else if (val == "some")
        options.setEmitAlignAttr(OptionLevel::SOME);
    else if (val == "all")
        options.setEmitAlignAttr(OptionLevel::ALL);
    else
        printHelpAndExit("Can't recognize emit-align-attr use level");
}

void OptionParser::parseUniqueAlignSize(std::string val) {
    Options &options = Options::getInstance();
    if (val.empty())
        options.setUniqueAlignSize(true);
    else if (val == "false")
        options.setUniqueAlignSize(false);
    else
        printHelpAndExit("Can't recognize unique align size");
}

void OptionParser::parseAlignSize(std::string val) {
    Options &options = Options::getInstance();
    if (val == "rand")
        return;
    else if (val == "16")
        options.setAlignSize(AlignmentSize::A16);
    else if (val == "32")
        options.setAlignSize(AlignmentSize::A32);
    else if (val == "64")
        options.setAlignSize(AlignmentSize::A64);
    else
        printHelpAndExit("Can't recognize alignment size");
    options.setUniqueAlignSize(true);
}

void OptionParser::parseAllowDeadData(std::string val) {
    Options &options = Options::getInstance();
    if (val == "true")
        options.setAllowDeadData(true);
    else if (val == "false")
        options.setAllowDeadData(false);
    else
        printHelpAndExit("Can't recognize allow dead data");
}

void OptionParser::parseEmitPragmas(std::string val) {
    Options &options = Options::getInstance();
    if (val == "none")
        options.setEmitPragmas(OptionLevel::NONE);
    else if (val == "some")
        options.setEmitPragmas(OptionLevel::SOME);
    else if (val == "all")
        options.setEmitPragmas(OptionLevel::ALL);
    else
        printHelpAndExit("Can't recognize emit-pragmas use level");
}

void OptionParser::parseOutDir(std::string val) {
    Options &options = Options::getInstance();
    options.setOutDir(std::move(val));
}

void OptionParser::parseUseParamShuffle(std::string val) {
    Options &options = Options::getInstance();
    if (val == "true")
        options.setUseParamShuffle(true);
    else if (val == "false")
        options.setUseParamShuffle(false);
    else
        printHelpAndExit("Can't recognize allow dead data");
}

void OptionParser::parseExplLoopParams(std::string val) {
    Options &options = Options::getInstance();
    if (val == "true")
        options.setExplLoopParams(true);
    else if (val == "false")
        options.setExplLoopParams(false);
    else
        printHelpAndExit("Can't recognize explicit loop parameters");
}

void OptionParser::parseMutationSeed(std::string mutation_seed_str) {
    std::stringstream arg_ss(mutation_seed_str);
    Options &options = Options::getInstance();
    uint64_t seed = 0;
    arg_ss >> seed;
    options.setMutationSeed(seed);
}

void OptionParser::parseMutationKind(std::string mutate_str) {
    Options &options = Options::getInstance();
    if (mutate_str == "none")
        options.setMutationKind(MutationKind::NONE);
    else if (mutate_str == "exprs")
        options.setMutationKind(MutationKind::EXPRS);
    else if (mutate_str == "all")
        options.setMutationKind(MutationKind::ALL);
    else
        printHelpAndExit("Can't recognize mutation parameters");
}

void OptionParser::parseAllowUBInDC(std::string allow_ub_in_dc_str) {
    Options &options = Options::getInstance();
    if (allow_ub_in_dc_str == "none")
        options.setAllowUBInDC(OptionLevel::NONE);
    else if (allow_ub_in_dc_str == "some")
        options.setAllowUBInDC(OptionLevel::SOME);
    else if (allow_ub_in_dc_str == "all")
        options.setAllowUBInDC(OptionLevel::ALL);
    else
        printHelpAndExit("Can't recognize input as arguments use level");
}

void Options::dump(std::ostream &stream) {
    dumpVersion(stream);
    stream << "Seed: " << seed << "\n";
    stream << "Invocation:";
    for (const auto &option : raw_options) {
        stream << " " << option;
    }
    stream << "\n";
}

void Options::setRawOptions(size_t argc, char *argv[]) {
    raw_options.reserve(argc);
    for (size_t i = 0; i < argc; ++i)
        raw_options.emplace_back(argv[i]);
}
