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

import subprocess
import argparse
import multiprocessing
import os
import shutil
import sys

yarpgen_home = os.environ["YARPGEN_HOME"]
sys.path.insert(0, yarpgen_home)
import run_gen
import brutus_blame

def test(num, file_queue, out_passed_queue, out_failed_queue, lock):
    compiler_passes = []
    wrap_exe = []
    fail_tag = []
    compiler_passes, wrap_exe, fail_tag = run_gen.fill_task(args.compiler)
    
    cwd_save = os.getcwd()

    while not file_queue.empty():
        test_seed_folder = file_queue.get()
        run_gen.print_debug("Job #" + str(num) + " | " + str(args.folder + test_seed_folder), args.verbose)
        os.chdir(cwd_save + os.sep + args.folder + os.sep + test_seed_folder)

        pass_res = set()
        failed_flag = False
        for i in range(len(compiler_passes)):
            ret_code, output = run_gen.run_cmd(num, compiler_passes[i], args.verbose)
            run_gen.print_debug("compile output: " + str(output), args.verbose)
            if (ret_code != 0):
                failed_flag = True
                break
            ret_code, output = run_gen.run_cmd(num, wrap_exe [i], args.verbose)
            run_gen.print_debug("run output: " + str(output), args.verbose)
            if (ret_code != 0):
                failed_flag = True
                break
            else:
                pass_res.add(output)
            if len(pass_res) > 1:
                failed_flag = True
                break
        if failed_flag:
            out_failed_queue.put(test_seed_folder)
            #brutus_blame.blame(cwd_save + os.sep + args.folder + test_seed_folder, yarpgen_home+os.sep+'found'+os.sep+"sort", args.verbose, lock)
        else:
            out_passed_queue.put(test_seed_folder)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script for re-checking fail folder after fix')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='enable verbose mode.')
    parser.add_argument('-f', '--timeout', dest='folder', type=str, default='.'+os.sep,
                        help='folder for re-check')
    parser.add_argument('-j', '--jobs', dest='num_jobs', default=multiprocessing.cpu_count(), type=int,
                        help='Maximum number of jobs to run in parallel')
    parser.add_argument('-c', '--compiler', dest='compiler', default="icc clang", type=str,
                        help='Test compilers')
    args = parser.parse_args()

    print("Re-check folder is " + args.folder)
    files = os.listdir(args.folder)
    file_queue = multiprocessing.Queue()
    run_gen.print_debug("Test files: ", args.verbose)
    for i in files:
        if (i.startswith("S_")):
          file_queue.put(i)
          run_gen.print_debug(i, args.verbose)
    
    lock = multiprocessing.Lock()

    out_passed_queue = multiprocessing.Queue()
    out_failed_queue = multiprocessing.Queue()

    task_threads = [0] * args.num_jobs
    for num in range(args.num_jobs):
        task_threads [num] =  multiprocessing.Process(target=test, args=(num, file_queue, out_passed_queue, out_failed_queue, lock))
        task_threads [num].start()

    for num in range(args.num_jobs):
        task_threads [num].join()

    print("Passed: ")
    while not out_passed_queue.empty():
        print(out_passed_queue.get())

    print("Failed: ")
    while not out_failed_queue.empty():
        print(out_failed_queue.get())
