#!/usr/bin/python2.7
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


import argparse
import os
import sys
import re

yarpgen_home = os.environ["YARPGEN_HOME"]
sys.path.insert(0, yarpgen_home)
import run_gen

def get_run_options (arg_folder, arg_verbose):
    cwd_save = os.getcwd()

    os.chdir(cwd_save + os.sep + arg_folder)

    compiler_passes = []
    wrap_exe = []
    fail_tag = []
    compiler_passes, wrap_exe, fail_tag = run_gen.fill_task("icc")

    valid_res = ""
    fail_option = ""
    fail_wrap_exe = ""
    pass_res = set()
    failed_flag = False
    for i in range(len(compiler_passes)):
        ret_code, output = run_gen.run_cmd(0, compiler_passes[i], arg_verbose)
        fail_option = output
        run_gen.print_debug("compile output: " + str(output), arg_verbose)
        if (ret_code != 0):
            failed_flag = True
            break
        ret_code, output = run_gen.run_cmd(0, wrap_exe [i], arg_verbose)
        fail_wrap_exe = wrap_exe [i]
        run_gen.print_debug("run output: " + str(output), arg_verbose)
        if (ret_code != 0):
            failed_flag = True
            break
        else:
            pass_res.add(output)
        if len(pass_res) > 1:
            failed_flag = True
            break
        else:
            valid_res = output
        
    if failed_flag:
        return valid_res, fail_option, fail_wrap_exe

def get_next_step (start, end, current, fail_flag):
    if (fail_flag):
        next_start = start
        next_current = (current - start) // 2 + start
        next_end = current
    else:
        next_start = current
        next_current = (end - current) // 2 + current
        next_end = end
    return next_start, next_end, next_current

def start_blame (valid_res, fail_option, fail_wrap_exe, blame_str, arg_verbose):
    eff = False # Earliest fail was found
    start_rtn = 0
    end_rtn = 10000
    cur_rtn = end_rtn // 2
    failed_flag = True
    time_to_finish = False
    while not time_to_finish:
        start_rtn, end_rtn, cur_rtn = get_next_step(start_rtn, end_rtn, cur_rtn, failed_flag)
        failed_flag = False
        #print (start_rtn, cur_rtn, end_rtn)
        eff = ((start_rtn + 1) == cur_rtn)
        ret_code, output = run_gen.run_cmd(0, ["bash", "-c", fail_option[:-1] + " " + blame_str + str(cur_rtn)], arg_verbose)
#        run_gen.print_debug("compile output: " + str(output), arg_verbose)
        if (ret_code != 0):
            failed_flag = True
            if (not eff):
                continue
            else:
                break
        ret_code, output = run_gen.run_cmd(0, fail_wrap_exe, arg_verbose)
        run_gen.print_debug("run output: " + str(output), arg_verbose)
        if (ret_code != 0):
            failed_flag = True
            if (not eff):
                continue
            else:
                break
        if (output != valid_res):
            failed_flag = True
            if (not eff):
                continue
            else:
                break
        time_to_finish = (eff and failed_flag) or (eff and not failed_flag and (cur_rtn == (end_rtn - 1)))
        #print (eff)
        #print (failed_flag)
        #print (time_to_finish)
    if (not failed_flag):
        cur_rtn = end_rtn
    return blame_str  + str(cur_rtn) + " "

def blame(arg_folder, arg_verbose):
    valid_res, fail_option, fail_wrap_exe = get_run_options (arg_folder, arg_verbose)
    rtn = start_blame  (valid_res, fail_option, fail_wrap_exe, "-from_rtn=0 -to_rtn=", arg_verbose)
    num_opt = start_blame  (valid_res, fail_option, fail_wrap_exe, rtn + "-num_opt=", arg_verbose)
    num_case = start_blame (valid_res, fail_option, fail_wrap_exe, num_opt + "-num-case=", arg_verbose)
    run_gen.print_debug(num_case, arg_verbose)
    ret_code, output = run_gen.run_cmd(0, ["bash", "-c", fail_option[:-1] + " " + num_case], arg_verbose)
#    print (output)
    blame_list = output.split("\n")
#    print(blame_list)
    blame_phase = ""
    for i in range(0,len(blame_list)):
        if ("DO ANOTHER" in blame_list[i]):
            blame_phase = blame_list[i]
        if ("NO MORE" in blame_list[i]):
            break;
    str_res = re.sub("DO ANOTHER ", "", blame_phase)
    str_res = re.sub(" \([0-9]*\)", "", str_res)
    run_gen.print_debug(str_res, arg_verbose)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script for brutus classification for ICC')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='enable verbose mode.')
    parser.add_argument('-f', '--folder', dest='folder', type=str, default='.'+os.sep,
                        help='source folder for check')
    parser.add_argument('-c', '--compiler', dest='compiler', default="icc", type=str,
                        help='Test compilers')
    args = parser.parse_args()

    blame(args.folder, args.verbose)
