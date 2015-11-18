#!/bin/python
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
import sys

def run_cmd (job_num, args):
    try:
        output = subprocess.check_output(args, stderr=subprocess.STDOUT)
        ret_code = 0
    except subprocess.CalledProcessError as e:
        print_debug ("Exception in run cmd in process " + str(job_num) + " with args:" + str(args))
        output = e.output
        ret_code = e.returncode
    return ret_code, output

def print_debug (line):
    if args.verbose:
        sys.stdout.write(str(line))
        sys.stdout.write("\n")
        sys.stdout.flush()

def test(num, file_queue, out_passed_queue, out_failed_queue):
        test_files = "init.cpp driver.cpp func.cpp check.cpp hash.cpp"
        icc_flags = "-std=c++11 -vec-threshold0 -restrict"
        clang_flags = "-std=c++11 -fslp-vectorize-aggressive"
        out_name = "out"

        compiler_passes = []
        wrap_exe = []
        compiler_passes.append(["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O0"])
        wrap_exe.append(out_name)
        compiler_passes.append(["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O3"])
        wrap_exe.append(out_name)

        cwd_save = os.getcwd()

        while not file_queue.empty():
            test_seed_folder = file_queue.get()
            print_debug("Job #" + str(num) + " | " + str(args.folder + test_seed_folder))
            os.chdir(cwd_save + os.sep + args.folder + test_seed_folder)

            pass_res = set()
            failed_flag = False
            for i in range(len(compiler_passes)):
                ret_code, output = run_cmd(num, compiler_passes[i])
                print_debug("compile output: " + str(output))
                if (ret_code != 0):
                    failed_flag = True
                    break
                ret_code, output = run_cmd(num, wrap_exe [i])
                print_debug("run output: " + str(output))
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
    args = parser.parse_args()

    print("Re-check folder is " + args.folder)
    files = os.listdir(args.folder)
    file_queue = multiprocessing.Queue()
    print_debug("Test files: ")
    for i in range(len(files)):
        file_queue.put(files[i])
        print_debug(files[i])
    
    out_passed_queue = multiprocessing.Queue()
    out_failed_queue = multiprocessing.Queue()

    task_threads = [0] * args.num_jobs
    for num in range(args.num_jobs):
        task_threads [num] =  multiprocessing.Process(target=test, args=(num, file_queue, out_passed_queue, out_failed_queue))
        task_threads [num].start()

    for num in range(args.num_jobs):
        task_threads [num].join()

    print("Passed: ")
    while not out_passed_queue.empty():
        print(out_passed_queue.get())

    print("Failed: ")
    while not out_failed_queue.empty():
        print(out_failed_queue.get())
