/*
Copyright (c) 2015-2020, Intel Corporation

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
#include <iostream>
#include <string>
#include <vector>

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

static void printVersion() {
    std::cout << "yarpgen version " << YARPGEN_VERSION_MAJOR << "."
              << YARPGEN_VERSION_MINOR << " (build " << BUILD_VERSION << " on "
              << BUILD_DATE << ")" << std::endl;
}

// This function prints out optional error_message, help and exits
static void printUsageAndExit(const std::string &error_msg = "") {
    int exit_code = 0;
    if (!error_msg.empty()) {
        std::cerr << error_msg << std::endl;
        exit_code = -1;
    }

    printVersion();
    std::cout << "usage: yarpgen\n";
    std::cout << "\t-h, --help                Display this message and exit\n";
    std::cout << "\t-v, --version             Print yarpgen version\n";
    exit(exit_code);
}

int main(int argc, char *argv[]) {
    std::vector<std::string> unused_args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
            printUsageAndExit();
        else if (arg == "-v" || arg == "--version")
            printVersion();
        else
            unused_args.push_back(arg);
    }

    if (!unused_args.empty()) {
        std::string unused_string = "Unused arguments: ";
        for (const auto &arg : unused_args)
            unused_string += arg + " ";
        printUsageAndExit(unused_string);
    }

    return 0;
}
