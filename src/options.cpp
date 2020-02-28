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

#include "options.h"
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

static const size_t PADDING = 25;

std::vector<OptionParser::option_t> yarpgen::OptionParser::opt_codes{
    {"-h", "--help", false, "Display help message", "Err",
     OptionParser::printHelpAndExit},
    {"-v", "--version", false, "Print YARPGen version", "Err",
     OptionParser::printVersion},
    {"-s", "--seed", true, "Pass a predefined seed", "Err",
     OptionParser::parseSeed},
};

void OptionParser::printVersion(std::string arg) {
    std::cout << "yarpgen version " << YARPGEN_VERSION_MAJOR << "."
              << YARPGEN_VERSION_MINOR << " (build " << BUILD_VERSION << " on "
              << BUILD_DATE << ")" << std::endl;
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
        if (num_printed > 0)
            std::cout << std::string(PADDING - num_printed, ' ');
        std::string output =
            item + (value ? "=<value>" : "") + (sep ? ", " : "");
        std::cout << output;
        return output.size();
    };

    for (auto &item : opt_codes) {
        size_t num_printed = 0;
        std::cout << "\t";
        num_printed += print_helper(std::get<0>(item));
        num_printed +=
            print_helper(std::get<1>(item), std::get<2>(item), false);
        print_helper(std::get<3>(item), false, false, num_printed);
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
                                 option_t option) {
    std::string short_arg = std::get<0>(option);
    bool has_value = std::get<2>(option);
    auto action = std::get<5>(option);
    if (!strcmp(argv[argv_iter], short_arg.c_str())) {
        if (has_value)
            argv_iter++;
        if (argv_iter == argc)
            printHelpAndExit(std::move(std::get<4>(option)));
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
                                option_t option) {
    std::string long_arg = std::get<1>(option);
    bool has_value = std::get<2>(option);
    auto action = std::get<5>(option);
    if (has_value)
        long_arg = long_arg + "=";
    if (optionStartsWith(argv[argv_iter], long_arg.c_str())) {
        if (has_value) {
            if (strlen(argv[argv_iter]) == long_arg.size())
                printHelpAndExit(std::move(std::get<4>(option)));
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
                printHelpAndExit(std::move(std::get<4>(option)));
        }
    }
    return false;
}

bool OptionParser::parseLongAndShortArgs(int argc, size_t &argv_iter,
                                         char **&argv, option_t option) {
    return parseLongArg(argv_iter, argv, option) ||
           parseShortArg(argc, argv_iter, argv, option);
}

size_t OptionParser::parseSeed(std::string seed_str) {
    std::stringstream arg_ss(seed_str);
    Options &options = Options::getInstance();
    size_t seed = 0;
    arg_ss >> seed;
    options.setSeed(seed);
    return 0;
}

void OptionParser::parse(size_t argc, char *argv[]) {
    for (size_t i = 1; i < argc; ++i) {
        bool parsed = false;
        for (const auto &item : opt_codes)
            if (parseLongAndShortArgs(argc, i, argv, item)) {
                parsed = true;
                break;
            }
        if (!parsed)
            printHelpAndExit("Unknown option: " + std::string(argv[i]));
    }
}
