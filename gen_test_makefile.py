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
# This scripts creates Test_Makefile, basing on configuration file
###############################################################################

import argparse
import datetime
import logging
import os
import sys
import re

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

default_config_file = "test_sets.txt"
comp_specs_line = "Compiler specs:"
spec_list_len = 3
test_sets_line = "Testing sets:"
set_list_len = 5

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

SdeArch = {}
# This list should be ordered!
SdeArch["p4"]  = SdeTarget("p4" , 0)
SdeArch["p4p"] = SdeTarget("p4p", 1)
SdeArch["mrm"] = SdeTarget("mrm", 2)
SdeArch["pnr"] = SdeTarget("pnr", 3)
SdeArch["nhm"] = SdeTarget("nhm", 4)
SdeArch["wsm"] = SdeTarget("wsm", 5)
SdeArch["snb"] = SdeTarget("snb", 6)
SdeArch["ivb"] = SdeTarget("ivb", 7)
SdeArch["hsw"] = SdeTarget("hsw", 8)
SdeArch["bdw"] = SdeTarget("bdw", 9)
SdeArch["skx"] = SdeTarget("skx", 10)
SdeArch["knl"] = SdeTarget("knl", 11)
SdeArch[""] = SdeTarget("", 12) # It is a fake target and it should always be the last

def define_sde_arch (native, target):
    if (target == SdeArch["skx"] and native != SdeArch["skx"]):
        return SdeArch["skx"].name
    if (target == SdeArch["knl"] and native != SdeArch["knl"]):
        return SdeArch["knl"].name
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
        self.version = "unknown"
        Compiler_specs.all_comp_specs [name] = self

    def set_version (self, version):
        self.version = version


class Arch (object):
    def __init__ (self, comp_name, sde_arch):
        self.comp_name = comp_name
        self.sde_arch = sde_arch


class Compiler_target (object):
    all_targets = []

    def __init__ (self, name, specs, args, arch):
        self.name = name
        self.specs = specs
        self.args = specs.common_args + " " + args
        self.arch = arch
        Compiler_target.all_targets.append(self)


###############################################################################
# Section for config parser

def skip_line(line):
    return line.startswith("#") or re.match(r'^\s*$', line)

def check_config_list (config_list, fixed_len, message):
    common.log_msg(logging.DEBUG, "Adding config list: " + str(config_list))
    if len(config_list) < fixed_len:
        common.print_and_exit(message + str(config_list))
    config_list = [x.strip() for x in config_list]
    return config_list

def add_specs (spec_list):
    spec_list = check_config_list(spec_list, spec_list_len, "Error in spec string, check it: ")
    try:
        Compiler_specs(spec_list [0], spec_list [1], spec_list [2])
        common.log_msg(logging.DEBUG, "Finished adding compiler spec")
    except KeyError:
        common.print_and_exit("Can't find key!")


def add_sets (set_list):
    set_list = check_config_list(set_list, set_list_len, "Error in set string, check it: ")
    try:
        Compiler_target(set_list [0], Compiler_specs.all_comp_specs [set_list [1]], set_list [2],
                        Arch(set_list [3], SdeArch[set_list [4]]))
        common.log_msg(logging.DEBUG, "Finished adding testing set")
    except KeyError:
        common.print_and_exit("Can't find key!")

def read_compiler_specs (config_iter, function, next_section_name = ""):
    for i in config_iter:
        if skip_line(i):
            continue
        if (next_section_name != "" and i.startswith(next_section_name)):
            return
        specs = i.split("|")
        function(specs)

def parse_config(file_name):
    config_file = common.check_and_open_file(file_name, "r")
    config = config_file.read().splitlines()
    config_file.close ()
    if not any(s.startswith(comp_specs_line) for s in config) or \
       not any(s.startswith(test_sets_line ) for s in config):
        common.print_and_exit("Invalid condig file! Check it!")
    config_iter = iter(config)
    for i in config_iter:
        if skip_line(i):
            continue
        if (i.startswith(comp_specs_line)):
            read_compiler_specs(config_iter, add_specs, test_sets_line)
            read_compiler_specs(config_iter, add_sets)

###############################################################################

def detect_native_arch():
    sys_compiler = ""
    for i in Compiler_specs.all_comp_specs:
        exec_name = Compiler_specs.all_comp_specs[i].comp_name
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
         common.print_and_exit("Can't compile " + check_isa_file + ": " + str(err_output, "utf-8"))
    ret_code, output, err_output, time_expired = common.run_cmd([check_isa_binary], None, 0)
    if (ret_code != 0):
        common.print_and_exit("Error while executing " + check_isa_binary)
    native_arch_str = str(output, "utf-8").split()[0]
    for i in SdeTarget.all_sde_targets:
        if (i.name == native_arch_str):
            return i
    common.print_and_exit("Can't detect system ISA")


def gen_makefile(out_file_name, force, verbose, config_file):
    parse_config(config_file)
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

    description = 'Generator of Test_Makefiles.'
    parser = argparse.ArgumentParser(description = description, formatter_class = argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--config-file", dest = "config_file", default = "test_sets.txt", type = str,
                        help = "Configuration file for testing")
    parser.add_argument("-o", "--output", dest = "out_file", default = "Test_Makefile", type = str,
                        help = "Output file")
    parser.add_argument("-f", "--force", dest = "force", default = False, action = "store_true",
                        help = "Rewrite output file")
    parser.add_argument("-v", "--verbose", dest = "verbose", default = False, action = "store_true", 
                        help = "Increase output verbosity")
    parser.add_argument("-efl", "--enable-file-logging", dest="enable_file_logging", default = False, action = "store_true",
                         help = "Enable logging to file.")
    default_log_file = "gen_test_makefile.log"
    parser.add_argument("--log-file", dest="log_file", default = default_log_file, type = str,
                        help = "Logfile")
    args = parser.parse_args()

    if (args.enable_file_logging or args.log_file != default_log_file):
        log_file = common.wrap_log_file(str(args.log_file), default_log_file)
        common.setup_logger(logger_name = common.file_logger_name, log_file = log_file, log_level = logging.DEBUG)

    log_level = logging.DEBUG if (args.verbose) else logging.ERROR
    common.setup_logger(logger_name = common.stderr_logger_name, log_level = log_level, write_to_stderr = True)

    common.check_python_version()
    gen_makefile(os.path.abspath(args.out_file), args.force, args.verbose, args.config_file)
