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
import datetime
import multiprocessing
import os
import sys
import subprocess
import shutil
import time

def print_debug (line):
    if args.verbose:
        sys.stdout.write(str(line))
        sys.stdout.write("\n")
        sys.stdout.flush()

def run_cmd (job_num, args):
    try:
        output = subprocess.check_output(args, stderr=subprocess.STDOUT)
        ret_code = 0
    except subprocess.CalledProcessError as e:
        print_debug ("Exception in run cmd in process " + str(job_num) + " with args:" + str(args))
        output = e.output
        ret_code = e.returncode
    return ret_code, output

def save_test (lock, gen_file, cmd, fail_type, output, seed):
    lock.acquire()
    str_seed = str(seed).split()[1][:-2]
    dest = ".." + os.sep + "result"
    pre_filter_str = []
    pre_filter_dir = []

    # ICC
    pre_filter_str.append("line 1224")
    pre_filter_dir.append("1224")

    pre_filter_str.append("line 159")
    pre_filter_dir.append("pcg")

    pre_filter_str.append("line 983")
    pre_filter_dir.append("uns_masked_op")

    pre_filter_str.append("L0 constraint violated")
    pre_filter_dir.append("const_viol")

    #CLANG
    pre_filter_str.append("any_extend")
    pre_filter_dir.append("any_extend")

    pre_filter_str.append("v4i64 = bitcast")
    pre_filter_dir.append("bitcast")

    pre_filter_str.append("i32 = X86ISD::CMP")
    pre_filter_dir.append("cmp")

    for i in range(len(pre_filter_str)):
        if (pre_filter_str [i] in str(output)):
            dest += os.sep + pre_filter_dir [i]

    dest += os.sep + "S_" + str_seed
    if (os.path.exists(dest)):
        log = open(dest + os.sep + "log.txt", "a")
        log.write("Type: " + str(fail_type) + "\n")
        log.write("Command: " + str(cmd) + "\n")
        log.write("Error: " + str(output) + "\n")
        lock.release()
        return
    else:
        os.makedirs(dest)
    print_debug("Test files were copied to " + dest)
    lock.release()
    log = open(dest + os.sep + "log.txt", "w")
    log.write("Seed: " + str(str_seed) + "\n")
    log.write("Time: " + datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S') + "\n")
    log.write("Type: " + str(fail_type) + "\n")
    log.write("Command: " + str(cmd) + "\n")
    log.write("Error: \n" + str(output) + "\n")
    log.close()
    for i in gen_file.split():
        shutil.copy(i, dest)



def gen_and_test(num, lock, end_time):
    print "Job #" + str(num)
    os.chdir(str(num))
    inf = end_time == -1

    test_files = "init.cpp driver.cpp func.cpp check.cpp hash.s"
    gen_file = test_files + " init.h" + " hash.cpp"
    out_name = "out"

    icc_flags = "-std=c++11 -vec-threshold0 -restrict"
    clang_flags = "-std=c++11 -fslp-vectorize-aggressive"

    compiler_passes = []
    wrap_exe = []
    compiler_passes.append(["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O0"])
    wrap_exe.append(out_name)
    compiler_passes.append(["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -O3"])
    wrap_exe.append(out_name)
    compiler_passes.append(["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -O0"])
    wrap_exe.append(out_name)
    compiler_passes.append(["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -O3"])
    wrap_exe.append(out_name)
    compiler_passes.append(["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -fsanitize=undefined -O3"])
    wrap_exe.append(out_name)
    compiler_passes.append(["bash", "-c", "icc " + test_files + " -o " + out_name + " " + icc_flags + " -xMIC-AVX512 -O3"])
    wrap_exe.append(["bash", "-c", "sde -knl -- " + "." + os.sep + out_name])
    compiler_passes.append(["bash", "-c", "clang++ " + test_files + " -o " + out_name + " " + clang_flags + " -march=knl -O3"])
    wrap_exe.append(["bash", "-c", "sde -knl -- " + "." + os.sep + out_name])

    subprocess.check_output(".." + os.sep + "yarpgen")
    hash_args = ["bash", "-c", "clang++ hash.cpp -S -o hash.s " + clang_flags + " -O3"]
    ret_code, output = run_cmd(num, hash_args)
    if (ret_code != 0):
        save_test(lock, gen_file, hash_args, "compfail", output , "hash-error")

    while inf or end_time > time.time():
        seed = ""
        seed = subprocess.check_output(".." + os.sep + "yarpgen")
        print_debug ("Job #" + str(num) + " " + seed)
        pass_res = set()
        for i in range(len(compiler_passes)):
#            print_debug("Job #" + str(num))
#            print_debug(str(i))
#            print_debug (str(compiler_passes[i]))
#            print_debug (str(wrap_exe[i]))
            ret_code, output = run_cmd(num, compiler_passes[i])
            if (ret_code != 0):
                save_test(lock, gen_file, compiler_passes[i], "compfail", output, seed)
                continue
            ret_code, output = run_cmd(num, wrap_exe [i])
            if (ret_code != 0):
                save_test(lock, gen_file, wrap_exe [i], "runfail", output, seed)
                continue
            else:
                pass_res.add(output)
            if len(pass_res) > 1:
                    print_debug (str(seed) + " " + str(compiler_passes[i]) + " " + str(wrap_exe[i]))
                    save_test (lock, gen_file, compiler_passes [i], "runfail", "output differs", seed)

def print_compiler_version():
    ret_code, output = run_cmd(-1, "which icc".split ())
    print_debug("ICC folder: " + output)
    ret_code, output = run_cmd(-1, "icc -v".split ())
    print_debug("ICC version: " + output)
    ret_code, output = run_cmd(-1, "which clang".split ())
    print_debug("clang folder: " + output)
    ret_code, output = run_cmd(-1, "clang -v".split ())
    print_debug("clang version: " + output)

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
