#!/usr/bin/python3
###############################################################################
#
# Copyright (c) 2015-2016, Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################
# We need some file to store compiler arguments. It can be stored in Makefile, but we need it in Python scripts.
# So we store them here and generate Test_Makefile
###############################################################################

import argparse
import logging
import os
import sys

import common

license_file_name = "LICENSE.txt"
check_isa_file_name = "check_isa.cpp"

time_exec = "/usr/bin/time"
time_output_format = '%U %S'
time_log_file_name = "time_log.txt"
time_args = ["-f", time_output_format, "-o", time_log_file_name, "-a"]

time_args_str = ""
need_quotes = False
for i in time_args:
    time_args_str += (i + " ") if (not need_quotes) else ('"' + i + '" ')
    need_quotes = True if (i == "-f") else False

time_run_str = time_exec + ' ' + time_args_str

###############################################################################
# Section for Test_Makefile parameters

class Makefile_variable ():
    '''A special class, which should link together name and value of parameters'''
    def __init__(self, name, value):
        self.name = name
        self.value = value

# I can't use build-in dictionary, because variables should be ordered
Makefile_variable_list = []

cxx_flags = Makefile_variable("CXXFLAGS", "-std=c++11")
Makefile_variable_list.append(cxx_flags)

ld_flags = Makefile_variable("LDFLAGS", "-std=c++11")
Makefile_variable_list.append(ld_flags)

sources = Makefile_variable("SOURCES", "init.cpp driver.cpp func.cpp check.cpp hash.cpp")
Makefile_variable_list.append(sources)

headers = Makefile_variable("HEADERS", "init.h")
Makefile_variable_list.append(headers)

executable = Makefile_variable("EXECUTABLE", "out")
Makefile_variable_list.append(executable)
#Makefile_variable_list.append(Makefile_variable("",""))

###############################################################################
# Section for sde

class SdeTarget (object):
    all_sde_targets = []

    def __init__ (self, name, enum_value):
        self.name = name
        self.enum_value = enum_value
        SdeTarget.all_sde_targets.append(self)

class SdeArch ():
    # This list should be ordered!
    p4  = SdeTarget("p4" , 0)
    p4p = SdeTarget("p4p", 1)
    mrm = SdeTarget("mrm", 2)
    pnr = SdeTarget("pnr", 3)
    nhm = SdeTarget("nhm", 4)
    wsm = SdeTarget("wsm", 5)
    snb = SdeTarget("snb", 6)
    ivb = SdeTarget("ivb", 7)
    hsw = SdeTarget("hsw", 8)
    bdw = SdeTarget("bdw", 9)
    skx = SdeTarget("skx", 10)
    knl = SdeTarget("knl", 11)
    native = SdeTarget("", 12) # It is a fake target and it should always be the last

def define_sde_arch (native, target):
    if (target == SdeArch.skx and native != SdeArch.skx):
        return SdeArch.skx.name
    if (target == SdeArch.knl and native != SdeArch.knl):
        return SdeArch.knl.name
    if (native.enum_value < target.enum_value):
        return target.name
    return ""

###############################################################################
# Section for targets

class Compiler_specs (object):
    all_comp_specs = dict()

    def __init__ (self, name, exec_name, common_args):
        self.name = name
        self.comp_name = exec_name
        self.common_args = common_args
        Compiler_specs.all_comp_specs [name] = exec_name

gcc_specs = Compiler_specs("gcc", "g++", "-w")
ubsan_specs = Compiler_specs("ubsan", "clang++", "-O0 -fsanitize=undefined -w")
clang_specs = Compiler_specs("clang", "clang++", "-w")

class Arch (object):
    def __init__ (self, comp_name, sde_arch):
        self.comp_name = comp_name
        self.sde_arch = sde_arch


class Compiler_target (object):
    all_targets = []

    def __init__ (self, name, specs, args, arch, target_list):
        self.name = name
        self.specs = specs
        self.args = specs.common_args + " " + args
        self.arch = arch
        Compiler_target.all_targets.append(self)
        target_list.append(self)

gcc_targets = []
ubsan_targets = []
clang_targets = []

ubsan = Compiler_target("ubsan", ubsan_specs, "",  Arch("", SdeArch.native), ubsan_targets)

gcc_no_opt     = Compiler_target("gcc_no_opt"    , gcc_specs, "-O0", Arch("nehalem", SdeArch.nhm)       , gcc_targets)
gcc_wsm_opt    = Compiler_target("gcc_wsm_opt"   , gcc_specs, "-O3", Arch("westmere", SdeArch.wsm)      , gcc_targets)
gcc_ivb_opt    = Compiler_target("gcc_ivb_opt"   , gcc_specs, "-O3", Arch("ivybridge", SdeArch.ivb)     , gcc_targets)
gcc_bdw_opt    = Compiler_target("gcc_bdw_opt"   , gcc_specs, "-O3", Arch("broadwell", SdeArch.bdw)     , gcc_targets)
gcc_knl_no_opt = Compiler_target("gcc_knl_no_opt", gcc_specs, "-O0", Arch("knl", SdeArch.knl)           , gcc_targets)
gcc_knl_opt    = Compiler_target("gcc_knl_opt"   , gcc_specs, "-O3", Arch("knl", SdeArch.knl)           , gcc_targets)
gcc_skx_no_opt = Compiler_target("gcc_skx_no_opt", gcc_specs, "-O0", Arch("skylake-avx512", SdeArch.skx), gcc_targets)
gcc_skx_opt    = Compiler_target("gcc_skx_opt"   , gcc_specs, "-O3", Arch("skylake-avx512", SdeArch.skx), gcc_targets)

clang_no_opt     = Compiler_target("clang_no_opt"    , clang_specs, "-O0",  Arch("", SdeArch.native), clang_targets)
clang_opt        = Compiler_target("clang_opt"       , clang_specs, "-O3",  Arch("", SdeArch.native), clang_targets)
clang_knl_no_opt = Compiler_target("clang_knl_no_opt", clang_specs, "-O0",  Arch("knl", SdeArch.knl), clang_targets)
clang_knl_opt    = Compiler_target("clang_knl_opt"   , clang_specs, "-O3",  Arch("knl", SdeArch.knl), clang_targets)
clang_skx_no_opt = Compiler_target("clang_skx_no_opt", clang_specs, "-O0",  Arch("skx", SdeArch.skx), clang_targets)
clang_skx_opt    = Compiler_target("clang_skx_opt"   , clang_specs, "-O3",  Arch("skx", SdeArch.skx), clang_targets)

###############################################################################

def detect_native_arch():
    sys_compiler = ""
    for i in Compiler_specs.all_comp_specs:
        exec_name = Compiler_specs.all_comp_specs[i]
        if common.if_exec_exist(exec_name):
            sys_compiler = exec_name
    if (sys_compiler == ""):
        common.print_and_exit("Can't find any compiler")

    check_isa_file = os.path.abspath(common.yarpgen_home + os.sep + check_isa_file_name)
    check_isa_binary = os.path.abspath(common.yarpgen_home + os.sep + check_isa_file_name.replace(".cpp", ""))
    if (not os.path.exists(check_isa_file)):
        common.print_and_exit("Can't find " + check_isa_file)
    ret_code, output, err_output, time_expired = common.run_cmd([sys_compiler, check_isa_file, "-o", check_isa_binary], None, 0)
    if (ret_code != 0):
         common.print_and_exit("Can't compile " + check_isa_file + ": " + str(err_output))
    ret_code, output, err_output, time_expired = common.run_cmd([check_isa_binary], None, 0)
    if (ret_code != 0):
        common.print_and_exit("Error while executing " + check_isa_binary)
    native_arch_str = str(output, "utf-8").split()[0]
    for i in SdeTarget.all_sde_targets:
        if (i.name == native_arch_str):
            return i
    common.print_and_exit("Can't detect system ISA")


def gen_makefile(out_file_name, force, verbose):
    output = ""
    license_file = common.check_and_open_file(os.path.abspath(common.yarpgen_home + os.sep + license_file_name), "r")
    for i in license_file:
        output += "#" + i
    output += "###############################################################################\n" 

    output += "#This file was generated automatically.\n"
    output += "#If you want to make a permanent changes, you should edit gen_test_makefile.py\n"
    output += "###############################################################################\n\n"

    for i in Makefile_variable_list:
        output += i.name + "=" + i.value + "\n"
    output += "\n"

    for i in Compiler_target.all_targets:
        output += i.name + ": " + "COMPILER=" + i.specs.comp_name + "\n"
        output += i.name + ": " + "OPTFLAGS=" + i.args
        if (i.arch.comp_name != ""):
            output += " -march=" + i.arch.comp_name + " "
        output += "\n"
        output += i.name + ": " + "EXECUTABLE=" + i.name + "_" + executable.value + "\n"
        output += i.name + ": " + "$(addprefix " + i.name + "_, $(SOURCES:.cpp=.o))\n"
        output += "\t" + time_run_str + "$(COMPILER) $(LDFLAGS) $(OPTFLAGS) -o $(EXECUTABLE) $^\n\n" 

    # Force make to rebuild everything
    #TODO: replace with PHONY
    output += "FORCE:\n\n"
    
    for i in sources.value.split():
        source_name = i.split(".") [0]
        output += "%" + source_name + ".o: "+ i + " FORCE\n"
        output += "\t" + time_run_str + "$(COMPILER) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $<\n\n"

    output += "clean:\n"
    output += "\trm *.o $(EXECUTABLE)\n\n"

    native_arch = detect_native_arch()
    for i in Compiler_target.all_targets:
        output += "run_" + i.name + ": " + i.name + "_" + executable.value + "\n"
        output += "\t" + time_run_str 
        required_sde_arch = define_sde_arch(native_arch, i.arch.sde_arch)
        if (required_sde_arch != ""):
            output += "sde -" + required_sde_arch + " -- "
        output += "." + os.sep + i.name + "_" + executable.value + "\n\n"

    if (not os.path.isfile(out_file_name)):
        out_file = open(out_file_name, "w")
    else:
        if (force):
            out_file = open(out_file_name, "w")
        else:
            common.print_and_exit("File already exists. Use -f if you want to rewrite it.")
    out_file.write(output)
    out_file.close()

###############################################################################

if __name__ == '__main__':
    if os.environ.get("YARPGEN_HOME") == None:
        sys.stderr.write("\nWarning: please set YARPGEN_HOME envirnoment variable to point to test generator path, using " + common.yarpgen_home + " for now\n")

    parser = argparse.ArgumentParser(description = 'Generator of Test_Makefiles.')
    parser.add_argument("-o", "--output", dest = "out_file", default = "Test_Makefile", type = str,
                        help = "Output file")
    parser.add_argument("-f", "--force", dest = "force", default = False, action = "store_true",
                        help = "Rewrite output file")
    parser.add_argument("-v", "--verbose", dest = "verbose", default = False, action = "store_true", 
                        help = "Increase output verbosity")
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)

    gen_makefile(os.path.abspath(args.out_file), args.force, args.verbose)
