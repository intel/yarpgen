#!/usr/local/bin/python2.7
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

yarpgen_home = os.environ["YARPGEN_HOME"]

def print_debug (line, verbose):
    if verbose:
        sys.stdout.write(str(line))
        sys.stdout.write("\n")
        sys.stdout.flush()

def run_cmd (job_num, args, verbose):
    try:
        output = subprocess.check_output(args, stderr=subprocess.STDOUT)
        ret_code = 0
    except subprocess.CalledProcessError as e:
        print_debug ("Exception in run cmd in process " + str(job_num) + " with args:" + str(args), verbose)
        output = e.output
        ret_code = e.returncode
    return ret_code, output

out_name = "out"

def fill_task(compiler):
    shutil.copy(yarpgen_home + os.sep + "Test_Makefile", ".")
    make_run_str = "make -f " + os.getcwd() + os.sep + "Test_Makefile "

    compiler_passes = []
    wrap_exe = []
    fail_tag = []
    if ("icc" in compiler):
        #compiler_passes.append(["bash", "-c", make_run_str + "icc_no_opt"])
        #wrap_exe.append(out_name)
        #fail_tag.append("icc" + os.sep + "run-uns")
        compiler_passes.append(["bash", "-c", make_run_str + "icc_opt"])
        wrap_exe.append(out_name)
        fail_tag.append("icc" + os.sep + "run-uns")
    if ("gcc" in compiler):
        #compiler_passes.append(["bash", "-c", make_run_str + "gcc_no_opt"])
        #wrap_exe.append(out_name)
        #fail_tag.append("gcc" + os.sep + "run-uns")
        compiler_passes.append(["bash", "-c", make_run_str + "gcc_wsm_opt"])
        wrap_exe.append(out_name)
        fail_tag.append("gcc" + os.sep + "run-uns")
        compiler_passes.append(["bash", "-c", make_run_str + "gcc_ivb_opt"])
        wrap_exe.append(out_name)
        fail_tag.append("gcc" + os.sep + "run-uns")
        compiler_passes.append(["bash", "-c", make_run_str + "gcc_bdw_opt"])
        wrap_exe.append(["bash", "-c", "sde -bdw -- " + "." + os.sep + out_name])
        fail_tag.append("gcc" + os.sep + "run-uns")
    if ("clang" in compiler):
        #compiler_passes.append(["bash", "-c", make_run_str + "clang_no_opt"])
        #wrap_exe.append(out_name)
        #fail_tag.append("clang" + os.sep + "run-uns")
        compiler_passes.append(["bash", "-c", make_run_str + "clang_opt"])
        wrap_exe.append(out_name)
        fail_tag.append("clang" + os.sep + "run-uns")
        #compiler_passes.append(["bash", "-c", make_run_str + "ubsan"])
        #wrap_exe.append(out_name)
        #fail_tag.append("gen")
    if ("icc" in compiler):
        compiler_passes.append(["bash", "-c", make_run_str + "icc_knl_opt"])
        wrap_exe.append(["bash", "-c", "sde -knl -- " + "." + os.sep + out_name])
        fail_tag.append("icc" + os.sep + "knl-runfail")
    if ("clang" in compiler):
        compiler_passes.append(["bash", "-c", make_run_str + "clang_knl_opt"])
        wrap_exe.append(["bash", "-c", "sde -knl -- " + "." + os.sep + out_name])
        fail_tag.append("clang" + os.sep + "knl-runfail")
    if ("gcc" in compiler):
        compiler_passes.append(["bash", "-c", make_run_str + "gcc_knl_opt"])
        wrap_exe.append(["bash", "-c", "sde -knl -- " + "." + os.sep + out_name])
        fail_tag.append("gcc" + os.sep + "knl-runfail")
        compiler_passes.append(["bash", "-c", make_run_str + "gcc_skx_opt"])
        wrap_exe.append(["bash", "-c", "sde -skx -- " + "." + os.sep + out_name])
        fail_tag.append("gcc" + os.sep + "skx-runfail")
    return compiler_passes, wrap_exe, fail_tag

def save_test (lock, gen_file, cmd, tag, fail_type, output, seed):
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

    pre_filter_str.append("line 19467")
    pre_filter_dir.append("stridestore")

    pre_filter_str.append("line 1883")
    pre_filter_dir.append("too_big")

    pre_filter_str.append("check_il_consistency failure")
    pre_filter_dir.append("check_il")

    #CLANG
    pre_filter_str.append("any_extend")
    pre_filter_dir.append("any_extend")

    pre_filter_str.append("v4i64 = bitcast")
    pre_filter_dir.append("bitcast")

    pre_filter_str.append("i32 = X86ISD::CMP")
    pre_filter_dir.append("cmp")

    pre_filter_str.append("llvm::sys::PrintStackTrace")
    pre_filter_dir.append("crash")

    pre_filter_str.append(" = masked_load<")
    pre_filter_dir.append("masked_load")

    pre_filter_str.append("= masked_store<")
    pre_filter_dir.append("masked_store")

    #GCC
    pre_filter_str.append("tree-vect-data-refs.c:889")
    pre_filter_dir.append("inter_889")

    for i in range(len(pre_filter_str)):
        if (pre_filter_str [i] in str(output)):
            dest += os.sep + pre_filter_dir [i]

    if not tag == "":
        dest += os.sep + tag

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
    print_debug("Test files were copied to " + dest, args.verbose)
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

test_files_env = " init.cpp driver.cpp check.cpp hash.cpp "
test_files = test_files_env + "func.cpp "
gen_file = test_files + " init.h"

def gen_and_test(num, lock, end_time):
    print "Job #" + str(num)
    os.chdir(str(num))
    inf = end_time == -1

    out_name = "out"

    compiler_passes = []
    wrap_exe = []
    fail_tag = []
    compiler_passes, wrap_exe, fail_tag = fill_task(args.compiler)

    while inf or end_time > time.time():
        seed = ""
        seed = subprocess.check_output(["bash", "-c", ".." + os.sep + "yarpgen -q"])
        print_debug ("Job #" + str(num) + " " + seed, args.verbose)
        pass_res = set()
        tag = ""
        for i in range(len(compiler_passes)):
#            print_debug("Job #" + str(num), args.verbose)
#            print_debug(str(i), args.verbose)
#            print_debug (str(compiler_passes[i]), args.verbose)
#            print_debug (str(wrap_exe[i]), args.verbose)
            ret_code, output = run_cmd(num, compiler_passes[i], args.verbose)
            if (ret_code != 0):
                save_test(lock, gen_file, compiler_passes[i], "", "compfail", output, seed)
                continue
            #print compiler_passes[i]
            ret_code, output = run_cmd(num, wrap_exe [i], args.verbose)
            if (ret_code != 0):
                save_test(lock, gen_file, wrap_exe [i], "", "runfail", output, seed)
                continue
            else:
                pass_res.add(output)
            if len(pass_res) > 1:
                if tag == "":
                    tag = fail_tag[i]
                print_debug (str(seed) + " " + str(compiler_passes[i]) + " " + str(wrap_exe[i]), args.verbose)
                save_test (lock, gen_file, compiler_passes [i], tag, "runfail", "output differs", seed)

def print_compiler_version():
    compilers = args.compiler.split()
    for i in compilers:
        ret_code, output = run_cmd(-1, ("which " + i).split (), args.verbose)
        print_debug(i + " folder: " + output, args.verbose)
        ret_code, output = run_cmd(-1, (i + " -v").split (), args.verbose)
        print_debug(i + " version: " + output, args.verbose)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Test system of random loop generator.')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='enable verbose mode.')
    parser.add_argument('-t', '--timeout', dest='timeout', type=int, default=1,
                        help='timeout for generator in hours. -1 means infinitive')
    parser.add_argument('-j', '--jobs', dest='num_jobs', default=multiprocessing.cpu_count(), type=int,
                        help='Maximum number of jobs to run in parallel')
    parser.add_argument('-f', '--folder', dest='folder', default="test", type=str,
                        help='Test folder')
    parser.add_argument('-c', '--compiler', dest='compiler', default="icc clang", type=str,
                        help='Test compilers')
    args = parser.parse_args()

    print_compiler_version()

#    if os.path.exists("test"):
#       shutil.rmtree("test")
    if not os.path.exists(args.folder):
        os.makedirs(args.folder)
    if os.path.exists("yarpgen"):
        shutil.copy("yarpgen", args.folder)
        shutil.copy("Test_Makefile", args.folder)
    if not os.path.exists((args.folder) + os.sep + "yarpgen"):
        print_debug ("No binary file of generator was found.", args.verbose)
        sys.exit(-1)
    os.chdir(args.folder)
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
