#!/usr/bin/python3
###############################################################################
#
# Copyright (c) 2015-2017, Intel Corporation
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
import collections
import configparser
import enum
import logging
import os
import sys
import re

import common

Test_Makefile_name = "Test_Makefile"
license_file_name = "LICENSE.txt"
check_isa_file_name = "check_isa.cpp"
default_test_sets_file_name = "test_sets.txt"

default_config_file = "test_sets.txt"
comp_specs_line = "Compiler specs:"
spec_list_len = 5
test_sets_line = "Testing sets:"
set_list_len = 5
stats_capt_opt_line = "Options for statistics' capture:"
stats_capt_opt_list_len = 2

default_section = "DEFAULT"

config_parser = configparser.ConfigParser(empty_lines_in_values=False, allow_no_value=True)
config_parser[default_section]["std"] = "-std"

###############################################################################
# Section for Test_Makefile parameters


class MakefileVariable:
    """A special class, which should link together name and value of parameters"""
    def __init__(self, name, value):
        self.name = name
        self.value = value

# I can't use build-in dictionary, because variables should be ordered
Makefile_variable_list = []

cxx_flags = MakefileVariable("CXXFLAGS", "")
Makefile_variable_list.append(cxx_flags)

ld_flags = MakefileVariable("LDFLAGS", "")
Makefile_variable_list.append(ld_flags)

std_flags = MakefileVariable("STDFLAGS", "-std=")
Makefile_variable_list.append(std_flags)

# File extension will be set later to match selected language standard in adjust_sources_to_standard()
sources = MakefileVariable("SOURCES", "driver func")
Makefile_variable_list.append(sources)

headers = MakefileVariable("HEADERS", "init.h")
Makefile_variable_list.append(headers)

executable = MakefileVariable("EXECUTABLE", "out")
Makefile_variable_list.append(executable)
# Makefile_variable_list.append(Makefile_variable("",""))

stat_options = MakefileVariable("STATFLAGS", "")
Makefile_variable_list.append(stat_options)

###############################################################################
# Section for language standards
@enum.unique
class StdID(enum.IntEnum):
    # Better to use enum.auto, but it is available only since python3.6
    C99 = 0
    C11 = 1
    MAX_C_ID = 2
    CXX98 = 3
    CXX03 = 4
    CXX11 = 5
    CXX14 = 6
    CXX17 = 7
    MAX_CXX_ID = 8

    def is_c (self):
        return StdID.C99.value <= self.value < StdID.MAX_C_ID.value

    def is_cxx (self):
        return StdID.CXX98.value <= self.value < StdID.MAX_CXX_ID.value

    ''' Enum doesn't allow to use '++' in names, so we need this function. '''
    @staticmethod
    def get_pretty_std_name (std_id):
        if std_id.is_c():
            return std_id.name.replace("C", "c")
        if std_id.is_cxx():
            return std_id.name.replace("CXX", "c++")

''' Easy way to convert string to StdID '''
StrToStdId = collections.OrderedDict()
for i in StdID:
    if not i.name.startswith("MAX"):
        StrToStdId[StdID.get_pretty_std_name(i)] = i

selected_standard = None

def get_file_ext():
    if selected_standard.is_c():
        return ".c"
    if selected_standard.is_cxx():
        return ".cpp"
    return None

def adjust_sources_to_standard():
    config_parser.set(default_section, "src_file_ext", get_file_ext())

def set_standard (std_str):
    global selected_standard
    global config_parser
    selected_standard = StrToStdId[std_str]
    std_string = config_parser.get(default_section, "std") + \
                 StdID.get_pretty_std_name(selected_standard)
    config_parser.set(default_section, "std", std_string)
    adjust_sources_to_standard()

def get_standard ():
    global selected_standard
    return StdID.get_pretty_std_name(selected_standard)

def check_if_std_defined ():
    if selected_standard is None or \
       selected_standard == StdID.MAX_C_ID or \
       selected_standard == StdID.MAX_CXX_ID:
        common.print_and_exit("Language standard wasn't selected!")

###############################################################################
# Section for sde


class SdeTarget (object):
    all_sde_targets = []

    def __init__(self, name, enum_value):
        self.name = name
        self.enum_value = enum_value
        SdeTarget.all_sde_targets.append(self)

SdeArch = dict()
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
SdeArch[""] = SdeTarget("", 12)  # It is a fake target and it should always be the last


def define_sde_arch(native, target):
    if target == SdeArch["skx"] and native != SdeArch["skx"]:
        return SdeArch["skx"].name
    if target == SdeArch["knl"] and native != SdeArch["knl"]:
        return SdeArch["knl"].name
    if native.enum_value < target.enum_value:
        return target.name
    return ""

###############################################################################
# Section for targets


class CompilerSpecs (object):
    all_comp_specs = dict()

    def __init__(self, name, cxx_exec_name, c_exec_name, common_args, arch_prefix):
        self.name = name
        self.comp_cxx_name = cxx_exec_name
        self.comp_c_name = c_exec_name
        self.common_args = common_args
        self.arch_prefix = arch_prefix
        self.version = "unknown"
        CompilerSpecs.all_comp_specs[name] = self

    def set_version(self, version):
        self.version = version


class Arch (object):
    def __init__(self, comp_name, sde_arch):
        self.comp_name = comp_name
        self.sde_arch = sde_arch


class CompilerTarget (object):
    all_targets = []

    def __init__(self, name, specs, target_args, arch):
        self.name = name
        self.specs = specs
        self.args = specs.common_args + " " + target_args
        self.arch = arch
        CompilerTarget.all_targets.append(self)


class StatisticsOptions (object):
    all_stats_options = dict()

    def __init__(self, spec, options):
        self.options = options
        StatisticsOptions.all_stats_options[spec.name] = self

    @staticmethod
    def get_options(spec):
        try:
            return StatisticsOptions.all_stats_options[spec.name].options
        except KeyError:
            common.print_and_exit("Can't find key!")

###############################################################################
# Section for config parser

def parse_config(file_name):
    # Before parsing, clean old data
    CompilerSpecs.all_comp_specs = dict()
    CompilerTarget.all_targets = []

    # Parse
    config_parser.read(file_name)
    set_standard(StdID.get_pretty_std_name(selected_standard))

    for section in config_parser.sections():
        if config_parser.has_option(section, "spec"):
            common.log_msg(logging.DEBUG, "Parsing spec: " + str(section))

            # Create CompilerSpecs
            cxx_exec_name = config_parser.get(section, "cxx_exec_name")
            c_exec_name = config_parser.get(section, "c_exec_name")
            CompilerSpecs(name=section, cxx_exec_name=cxx_exec_name,
                                  c_exec_name=c_exec_name,
                                  common_args=config_parser.get(section, "spec_common_args"),
                                  arch_prefix=config_parser.get(section, "arch_prefix"))

            # Set exec_name in config_parser
            common.log_msg(logging.DEBUG, "Set exec_name")
            if selected_standard.is_cxx():
                config_parser.set(section, "exec_name", cxx_exec_name)
            if selected_standard.is_c():
                config_parser.set(section, "exec_name", c_exec_name)

        elif config_parser.has_option(section, "test_set"):
            common.log_msg(logging.DEBUG, "Parsing test set: " + str(section))

            # Set arch parameters in config_parser
            common.log_msg(logging.DEBUG, "Set arch params")
            comp_arch_option = (section, "comp_arch")
            comp_arch = ""
            if config_parser.has_option(*comp_arch_option):
                comp_arch = config_parser.get(*comp_arch_option)
            else:
                config_parser.set(section, "arch_str", "")

            sde_arch_option = (section, "sde_arch")
            sde_arch = ""
            if config_parser.has_option(*sde_arch_option):
                sde_arch = config_parser.get(*sde_arch_option)
            else:
                config_parser.set(section, "sde_prefix", "")

            # Set test prefix
            config_parser[section]["test_set_prefix"] = section

            # Import all options from corr_spec options to test_set section (needed for correct interpolation)
            common.log_msg(logging.DEBUG, "Import all options from corr_spec to test_set section")
            corr_spec = config_parser.get(section, "corr_spec")
            for j in config_parser.options(corr_spec):
                if j == "spec":
                    continue
                if not config_parser.has_option(section, j) or config_parser.get(section, j, raw=True) is None:
                    config_parser[section][j] = config_parser.get(corr_spec, j, raw=True)

            # Create CompilerTarget
            CompilerTarget(name=section, specs=CompilerSpecs.all_comp_specs[corr_spec],
                                   target_args=config_parser.get(section, "test_set_args"),
                                   arch=Arch(comp_arch, SdeArch[sde_arch]))
        else:
            common.print_and_exit("Invalid config file! Check it! "
                                  "It may consist of specs, test_sets and default section")

###############################################################################


def detect_native_arch():
    check_isa_file = os.path.abspath(common.yarpgen_home + os.sep + check_isa_file_name)
    check_isa_binary = os.path.abspath(common.yarpgen_home + os.sep + check_isa_file_name.replace(".cpp", ""))

    sys_compiler = ""
    for key in CompilerSpecs.all_comp_specs:
        exec_name = CompilerSpecs.all_comp_specs[key].comp_cxx_name
        if common.if_exec_exist(exec_name):
            sys_compiler = exec_name
            break
    if sys_compiler == "":
        common.print_and_exit("Can't find any compiler")

    if not common.if_exec_exist(check_isa_binary):
        if not os.path.exists(check_isa_file):
            common.print_and_exit("Can't find " + check_isa_file)
        ret_code, output, err_output, time_expired, elapsed_time = \
            common.run_cmd([sys_compiler, check_isa_file, "-o", check_isa_binary], None)
        if ret_code != 0:
            common.print_and_exit("Can't compile " + check_isa_file + ": " + str(err_output, "utf-8"))

    ret_code, output, err_output, time_expired, elapsed_time = common.run_cmd([check_isa_binary], None)
    if ret_code != 0:
        common.print_and_exit("Error while executing " + check_isa_binary)
    native_arch_str = str(output, "utf-8").split()[0]
    for sde_target in SdeTarget.all_sde_targets:
        if sde_target.name == native_arch_str:
            return sde_target
    common.print_and_exit("Can't detect system ISA")


def gen_makefile(out_file_name, force, config_file, only_target=None, blame_args=None,
                 creduce_file=None, stat_targets=None):
    # Somebody can prepare test specs and target, so we don't need to parse config file
    common.log_msg(logging.DEBUG, "Trying to generate makefile")
    check_if_std_defined()
    if config_file is not None:
        parse_config(config_file)
    output = ""

    if stat_targets is not None:
        stat_targets = list(set(stat_targets))

    # 1. License
    license_file = common.check_and_open_file(os.path.abspath(common.yarpgen_home + os.sep + license_file_name), "r")
    for license_str in license_file:
        output += "#" + license_str
    license_file.close()
    output += "###############################################################################\n"

    output += "#This file was generated automatically.\n"
    output += "#If you want to make a permanent changes, you should edit gen_test_makefile.py\n"
    output += "###############################################################################\n\n"

    # 2. Process all targets
    common.log_msg(logging.DEBUG, "Processing targets")
    for target in CompilerTarget.all_targets:
        if only_target is not None and only_target.name != target.name:
            continue

        output += target.name + ":\n"
        # Emit driver compilation
        output += "\t" + config_parser.get(target.name, "driver_compilation") + "\n"

        # Emit test compilation stages
        comp_stage_num = 1
        comp_stage_str = "comp_stage_" + str(comp_stage_num)
        while config_parser.has_option(target.name, comp_stage_str):
            common.log_msg(logging.DEBUG, "Processing compilation stage #" + str(comp_stage_num))
            # Merge compilation stage arguments if they exist
            comp_stage_arg_str = "comp_stage_" + str(comp_stage_num) + "_args"
            if config_file is not None and config_parser.has_option(target.name, comp_stage_arg_str):
                comp_stage = config_parser.get(target.name, comp_stage_str, raw=True)
                comp_stage_arg = config_parser.get(target.name, comp_stage_arg_str, raw=True)
                config_parser.set(target.name, comp_stage_str, comp_stage + " " + comp_stage_arg)

            comp_stage_stat_str = "comp_stage_" + str(comp_stage_num) + "_stat"
            if stat_targets is not None and target.name in stat_targets:
                if not config_parser.has_option(target.name, comp_stage_stat_str):
                    common.print_and_exit("Can't find stat args for target " + str(target.name))
                comp_stage = config_parser.get(target.name, comp_stage_str, raw=True)
                comp_stage_stat = config_parser.get(target.name, comp_stage_stat_str, raw=True)
                config_parser.set(target.name, comp_stage_str, comp_stage + " " + comp_stage_stat)

            resulting_comp_stage_blame_args_str = " "
            if blame_args is not None and blame_args[comp_stage_num] is not None:
                comp_stage_blame_str = "comp_stage_" + str(comp_stage_num) + "_blame_args"
                comp_stage_blame_arg = ""
                if config_parser.has_option(target.name, comp_stage_blame_str):
                    comp_stage_blame_arg = config_parser.get(target.name, comp_stage_blame_str, raw=True)
                comp_stage_blame_arg_list = comp_stage_blame_arg.split("|")
                for blame_arg_num in range(len(comp_stage_blame_arg_list)):
                    blame_arg = blame_args[comp_stage_num][blame_arg_num]
                    if blame_arg is None:
                        continue
                    resulting_comp_stage_blame_args_str += comp_stage_blame_arg_list[blame_arg_num].strip() \
                                                           + str(blame_arg)

            # Emit complete compilation stage string
            output += "\t" + config_parser.get(target.name, comp_stage_str) + resulting_comp_stage_blame_args_str + "\n"

            # Next iteration
            comp_stage_num += 1
            comp_stage_str = "comp_stage_" + str(comp_stage_num)

        if stat_targets is not None and target.name in stat_targets and \
           config_parser.has_option(target.name, "merge_stats_files"):
            output += "\t" + config_parser.get(target.name, "merge_stats_files") + "\n"

        # Emit linking
        output += "\t" + config_parser.get(target.name, "linking") + "\n"
        output += "\n"

        # Emit run string
        output += "run_" + target.name + ": " + config_parser.get(target.name, "out_exec") + "\n"
        output += "\t@" + config_parser.get(target.name, "execution") + "\n"
        output += "\n"

    # 3. Print clean target
    output += "clean:\n"
    output += "\trm *.o *_" + config_parser.get(default_section, "out_exec_suffix") + "\n\n"

    out_file = None
    if not os.path.isfile(out_file_name):
        out_file = open(out_file_name, "w")
    else:
        if force:
            out_file = open(out_file_name, "w")
        else:
            common.print_and_exit("File already exists. Use -f if you want to rewrite it.")
    out_file.write(output)
    out_file.close()


def get_blame_args (blame_target, num):
    common.log_msg(logging.DEBUG, "Trying to match blame target")
    cant_blame = True
    for target in CompilerTarget.all_targets:
        if blame_target.name == target.name:
            cant_blame = False
            break

    if cant_blame:
        common.log_msg(logging.DEBUG, "We can't blame " + blame_target.name + " (process " + str(num) + ")")
        return None

    blame_args_list = {}
    comp_stage_num = 1
    comp_stage_str = "comp_stage_" + str(comp_stage_num)
    while config_parser.has_option(blame_target.name, comp_stage_str):
        blame_args_list[comp_stage_num] = None
        comp_stage_blame_arg_str = "comp_stage_" + str(comp_stage_num) + "_blame_args"
        if config_parser.has_option(blame_target.name, comp_stage_blame_arg_str):
            comp_stage_blame_arg = config_parser.get(blame_target.name, comp_stage_blame_arg_str, raw=True)
            comp_stage_blame_arg_list = comp_stage_blame_arg.split("|")
            blame_args_list[comp_stage_num] = ([-1] * len(comp_stage_blame_arg_list))
        comp_stage_num += 1
        comp_stage_str = "comp_stage_" + str(comp_stage_num)
    common.log_msg(logging.DEBUG, "Blame args list: " + str(blame_args_list))

    return blame_args_list

###############################################################################


if __name__ == '__main__':
    if os.environ.get("YARPGEN_HOME") is None:
        sys.stderr.write("\nWarning: please set YARPGEN_HOME envirnoment variable to point to test generator path, "
                         "using " + common.yarpgen_home + " for now\n")

    description = 'Generator of Test_Makefiles.'
    parser = argparse.ArgumentParser(description=description, formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--std', dest="std_str", default="c++11", type=str,
                        help='Language standard. Possible variants are ' + str(list(StrToStdId))[1:-1])

    parser.add_argument("--config-file", dest="config_file",
                        default=os.path.join(common.yarpgen_home, default_test_sets_file_name), type=str,
                        help="Configuration file for testing")
    parser.add_argument("-o", "--output", dest="out_file", default=Test_Makefile_name, type=str,
                        help="Output file")
    parser.add_argument("-f", "--force", dest="force", default=False, action="store_true",
                        help="Rewrite output file")
    parser.add_argument("-v", "--verbose", dest="verbose", default=False, action="store_true",
                        help="Increase output verbosity")
    parser.add_argument("--log-file", dest="log_file", type=str,
                        help="Logfile")
    parser.add_argument("--creduce-file", dest="creduce_file", default=None, type=str,
                        help="Source file to reduce")
    parser.add_argument("--collect-stat", dest="collect_stat", default="", type=str,
                        help="List of testing sets for statistics collection")
    args = parser.parse_args()

    log_level = logging.DEBUG if args.verbose else logging.INFO
    common.setup_logger(args.log_file, log_level)

    common.check_python_version()
    set_standard(args.std_str)
    gen_makefile(os.path.abspath(args.out_file), args.force, args.config_file, creduce_file=args.creduce_file,
                 stat_targets=args.collect_stat.split())
