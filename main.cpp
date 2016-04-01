/*
Copyright (c) 2015-2016, Intel Corporation

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

#include <getopt.h>
#include <cstdint>
#include <iostream>
#include "type.h"

using namespace rl;

extern void self_test();

int main (int argc, char* argv[]) {
/*
    extern char *optarg;
    extern int optind;
    char *pEnd;
    std::string out_dir = "./";
    int c;
    uint64_t seed = 0;
    static char usage[] = "usage: [-q -d <out_dir> -s <seed>\n";
    bool opt_parse_err = 0;
    bool quiet = false;

    while ((c = getopt(argc, argv, "qhrd:s:")) != -1)
        switch (c) {
        case 'd':
            out_dir = std::string(optarg);
            break;
        case 's':
            seed = strtoull(optarg, &pEnd, 10);
            break;
        case 'q':
            quiet = true;
            break;
        case 'h':
        default:
            opt_parse_err = true;
            break;
        }
    if (optind < argc) {
        for (; optind < argc; optind++)
            std::cerr << "Unrecognized option: " << argv[optind] << std::endl;
        opt_parse_err = true;
    }
    if (argc == 1 && !quiet) {
        std::cerr << "Using default options" << std::endl;
        std::cerr << "For help type " << argv [0] << " -h" << std::endl;
    }
    if (opt_parse_err) {
        std::cerr << usage << std::endl;
        exit(-1);
    }
*/
    self_test();

    return 0;
}
