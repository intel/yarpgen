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
import multiprocessing
import os
import sys
import shutil
import re

yarpgen_home = os.environ["YARPGEN_HOME"]
sys.path.insert(0, yarpgen_home)
import run_gen

cwd_save = os.getcwd()

def get_run_options (arg_folder, arg_verbose):
    cwd_save = os.getcwd()
    if (os.path.normpath(cwd_save) != os.path.normpath(arg_folder)):
        os.chdir(cwd_save + os.sep + arg_folder)
    if os.path.exists("yarpgen"):
        shutil.copy(os.environ["YARPGEN_HOME"] + os.sep + "Test_Makefile", os.getcwd())

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
        fail_option = (output.split("\n"))[0]
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
        for i in run_gen.gen_file.split():
            fail_option = fail_option.replace(i, "")
        fail_option = fail_option.replace("-o " + run_gen.out_name, "")
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
    end_rtn = 100000
    cur_rtn = end_rtn // 2
    failed_flag = True
    time_to_finish = False
    while not time_to_finish:
        start_rtn, end_rtn, cur_rtn = get_next_step(start_rtn, end_rtn, cur_rtn, failed_flag)
        failed_flag = False
        #print (start_rtn, cur_rtn, end_rtn)
        eff = ((start_rtn + 1) == cur_rtn)
        ret_code, output = run_gen.run_cmd(0, ["bash", "-c", fail_option + " -c " + run_gen.test_files_env], arg_verbose)
        ret_code, output = run_gen.run_cmd(0, ["bash", "-c", fail_option + " -c func.cpp " + blame_str + str(cur_rtn)], arg_verbose)
#        run_gen.print_debug("compile output: " + str(output), arg_verbose)
        if (ret_code != 0):
            failed_flag = True
            if (not eff):
                continue
            else:
                break
        ret_code, output = run_gen.run_cmd(0, ["bash", "-c", fail_option + " -o " + run_gen.out_name + run_gen.test_files.replace(".cpp", ".o")], arg_verbose)
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
    return blame_str + str(cur_rtn) + " "

def blame(arg_folder, arg_res_folder, arg_verbose, lock):
    valid_res, fail_option, fail_wrap_exe = get_run_options (arg_folder, arg_verbose)
    rtn = start_blame  (valid_res, fail_option, fail_wrap_exe, "-from_rtn=0 -to_rtn=", arg_verbose)
    num_opt = start_blame  (valid_res, fail_option, fail_wrap_exe, rtn + "-num_opt=", arg_verbose)
    num_case = start_blame (valid_res, fail_option, fail_wrap_exe, num_opt + "-num-case=", arg_verbose)
    run_gen.print_debug(num_case, arg_verbose)
    ret_code, output = run_gen.run_cmd(0, ["bash", "-c", fail_option + " -c func.cpp " + num_case], arg_verbose)
#    print (output)
    blame_list = output.split("\n")
#    print(blame_list)
    blame_phase = ""
    str_res = ""
    for i in range(0,len(blame_list)):
        if ("(last opt)" in blame_list[i]):
            blame_phase = blame_list[i]
            str_res = re.sub("DOING ", "", blame_phase)
            str_res = re.sub(" (last opt)", "", str_res)
        if ("DO ANOTHER" in blame_list[i]):
            blame_phase = blame_list[i]
            str_res = re.sub("DO ANOTHER ", "", blame_phase)
        if ("NO MORE" in blame_list[i]):
            break;
    str_res = re.sub(" \([0-9]*\)", "", str_res)
    str_res = str_res.replace(" ", "_")
    run_gen.print_debug(str_res, arg_verbose)
    phase_str = fail_option + " -c " + run_gen.test_files_env + "\n"
    phase_str += fail_option + " -c func.cpp " + num_case + "\n"
    phase_str += fail_option + " -o " + run_gen.out_name + run_gen.test_files.replace(".cpp", ".o") + "\n"
    save_res(str_res, phase_str, arg_folder, arg_res_folder, arg_verbose, lock)

def save_res(filter_str, phase_str, arg_folder, arg_res_folder, arg_verbose, lock):
    lock.acquire()
    #dest = os.path.normpath(cwd_save + os.sep + arg_res_folder) + os.sep + str(filter_str)
    dest = arg_res_folder + os.sep + str(filter_str)
    #print (dest)
    seed_str_list = arg_folder.split(os.sep)
    seed_str = ""
    for i in seed_str_list:
        if (i.startswith("S_")):
            seed_str = i
            break
    if (not os.path.exists(dest)):
        os.makedirs(dest)
    dest += os.sep + str(seed_str)
    if (os.path.exists(dest)):
        log = open(dest + os.sep + "log.txt", "a")
        log.write(phase_str + "\n")
        log.close()
        lock.release()
        return
    else:
        os.makedirs(dest)
    copy_files = run_gen.gen_file.split()
    copy_files.append("log.txt")
    for i in copy_files:
        shutil.copy(i, dest)
    log = open(dest + os.sep + "log.txt", "a")
    log.write(phase_str + "\n")
    log.close()
    lock.release()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script for brutus classification for ICC')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='enable verbose mode.')
    parser.add_argument('-f', '--folder', dest='folder', type=str, default='.'+os.sep,
                        help='source folder for check')
    parser.add_argument('-r', '--res-folder', dest='res_folder', type=str, default=yarpgen_home+os.sep+'found'+os.sep+"sort",
                        help='folder for results')
    parser.add_argument('-c', '--compiler', dest='compiler', default="icc", type=str,
                        help='Test compilers')
    args = parser.parse_args()

    lock = multiprocessing.Lock()

    blame(args.folder, args.res_folder, args.verbose, lock)
