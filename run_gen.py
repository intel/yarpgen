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


import argparse
import multiprocessing
import os
import sys
import subprocess
import shutil
import time

def print_debug (line):
    if args.verbose:
        sys.stdout.write(line + "\n")
        sys.stdout.flush()

def run_cmd (job_num, args):
    try:
        return subprocess.check_output(args)
    except:
        print_debug ("Exception in run cmd in process " + str(job_num) + " with args:" + str(args))
        raise

def save_test (lock, gen_file):
    lock.acquire()
    folders = os.listdir(".." + os.sep + "result")
    folders.sort()
    if not folders:
        dest = ".." + os.sep + "result" + os.sep + str(1)
    else:
        dest = ".." + os.sep + "result" + os.sep + str(int(folders[-1]) + 1)
    print_debug("Test files were copied to " + dest)
    os.makedirs(dest)
    lock.release()
    for i in gen_file.split():
        shutil.copy(i, dest)



def gen_and_test(num, lock, end_time):
    print "Job #" + str(num)
    os.chdir(str(num))
    inf = end_time == -1

    test_files = "init.cpp driver.cpp func.cpp check.cpp hash.s"
    gen_file = test_files + " init.h" + "hash.cpp"
    out_name = "out"

    icc_flags = "-std=c++11 -vec-threshold0 -restrict"
    clang_flags = "-std=c++11"

    subprocess.check_output(".." + os.sep + "yarpgen")
    try:
        run_cmd(num, ["bash", "-c", "clang++ hash.cpp -S -o hash.s " + clang_flags + " -O3"])
    except:
        save_test(lock, gen_file)

    while inf or end_time > time.time():
        seed = ""
        seed = subprocess.check_output(".." + os.sep + "yarpgen")
        print_debug ("Job #" + str(num) + " " + seed)

        try:
            run_cmd(num, ["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O0"])
            icc_no_opt = run_cmd (num, [out_name])
            run_cmd(num, ["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O3"])
            icc_opt = run_cmd (num, [out_name])
        except:
            save_test(lock, gen_file)
            continue

        diag = ""
        if icc_no_opt != icc_opt:
            diag += str(seed) + "\n" + "ICC opt differs from ICC no-opt\n"

        try:
            run_cmd(num, ["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -O0"]) 
            clang_no_opt = run_cmd (num, [out_name])
            run_cmd(num, ["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -O3"])
            clang_opt = run_cmd (num, [out_name])
        except:
            save_test(lock, gen_file)
            continue

        if clang_no_opt != clang_opt:
            diag += str(seed) + "\n" + "CLANG opt differs from CLANG no-opt\n"

        if clang_opt != icc_opt:
            diag += str(seed) + "\n" + "CLANG opt differs from ICC opt\n"

        if diag:
            print_debug (diag)
            save_test (lock, gen_file)
 
def print_compiler_version():
    sp = run_cmd(-1, "which icc".split ())
    print_debug("ICC folder: " + sp)
    sp = run_cmd(-1, "icc -v".split ())
    sp = run_cmd(-1, "which clang".split ())
    print_debug("clang folder: " + sp)
    sp = run_cmd(-1, "clang -v".split ())

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Test system of random loop generator.')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='enable verbose mode.')
    parser.add_argument('-t', '--timeout', dest='timeout', type=int, default=1,
                        help='timeout for generator in hours. -1 means infinitive')
    parser.add_argument('-j', '--jobs', dest='num_jobs', default=multiprocessing.cpu_count(), type=int,
                        help='Maximum number of jobs to run in parallel')
    args = parser.parse_args()

    print_compiler_version()

#    if os.path.exists("test"):
#       shutil.rmtree("test")
    if not os.path.exists("test"):
        os.makedirs("test")
    if os.path.exists("yarpgen"):
        shutil.copy("yarpgen", "test")
    else:
        print_debug ("No binary file of generator was found.")
        sys.exit(-1)
    os.chdir("test")
    if not os.path.exists("result"):
        os.makedirs("result")
    for i in range(args.num_jobs):
        if not os.path.exists(str(i)):
            os.makedirs(str(i))

    lock = multiprocessing.Lock()

    start_time = time.time()
    end_time = start_time + args.timeout * 3600
    if args.timeout == -1:
        end_time = -1

    task_threads = [0] * args.num_jobs
    for num in range(args.num_jobs):
        task_threads [num] =  multiprocessing.Process(target=gen_and_test, args=(num, lock, end_time))
        task_threads [num].start()

    for num in range(args.num_jobs):
        task_threads [num].join()

    for i in range(args.num_jobs):
        shutil.rmtree(str(i))

'''
    while end_time > time.time():
        seed = ""
        if os.path.exists(".." + os.sep + "yarpgen"):
            seed = subprocess.check_output(".." + os.sep + "yarpgen")
        else:
            print_debug ("No binary file of generator was found.")
            sys.exit(-1)
        print_debug (seed)

        test_files = "init.cpp driver.cpp func.cpp check.cpp"
        gen_file = test_files + " init.h"
        out_name = "out"

        icc_flags = "-std=c++11 -vec-threshold0 -restrict"
        subprocess.check_output(["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O0"])
        icc_no_opt = subprocess.check_output([out_name])
        subprocess.check_output(["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O3"])
        icc_opt = subprocess.check_output([out_name])

        diag = ""
        if icc_no_opt != icc_opt:
            diag += str(seed) + "\n" + "ICC opt differs from ICC no-opt\n"

        clang_flags = "-std=c++11"
        subprocess.check_output(["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -O0"])
        clang_no_opt = subprocess.check_output([out_name])
        subprocess.check_output(["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -O3"])
        clang_opt = subprocess.check_output([out_name])

        if clang_no_opt != clang_opt:
            diag += str(seed) + "\n" + "CLANG opt differs from CLANG no-opt\n"

        if clang_opt != icc_opt:
            diag += str(seed) + "\n" + "CLANG opt differs from ICC opt\n"

        if diag:
            folders = os.listdir("result")
            folders.sort()  
            if not folders:
                dest = "result" + os.sep + str(1)
            else:
                dest = "result" + os.sep + str(int(folders[-1]) + 1)
            print_debug(diag + "Test files were copied to " + dest)
            os.makedirs(dest)
            for i in gen_file.split():
               shutil.copy(i, dest)
'''
