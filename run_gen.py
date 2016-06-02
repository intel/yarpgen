#!python3
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
    elapsed = -1
    start = time.time()
    
    try:
        output = subprocess.check_output(args, stderr=subprocess.STDOUT)
        elapsed = (time.time() - start)
        ret_code = 0
    except subprocess.CalledProcessError as e:
        elapsed = (time.time() - start)
        print_debug ("Exception in run cmd in process " + str(job_num) + " with args:" + str(args), verbose)
        output = e.output
        ret_code = e.returncode
    return ret_code, output, elapsed


def get_sde_target(target):
    if ("bdw" in target):
        return "bdw"
    if ("knl" in target):
        return "knl"
    if ("skx" in target):
        return "skx"
    return "unsupported!"


def add_task(exe_name_base, make_run_str, make_target, compiler_passes, wrap_exe, native=True):
    out_name = exe_name_base + "_" + make_target
    compiler_passes.append(["bash", "-c", make_run_str + out_name + " " + make_target])
    if (native):
        wrap_exe.append(out_name)
    else:
        wrap_exe.append(["bash", "-c", "sde -" + get_sde_target(make_target) 
                                     + " -- " + "." + os.sep + out_name])


def fill_task(compiler, task):
    out_name_base = "out_" + str(task)

    shutil.copy(yarpgen_home + os.sep + "Test_Makefile", ".")
    make_run_str = "make -f " + os.getcwd() + os.sep + "Test_Makefile "
    make_run_str += "EXECUTABLE="

    compiler_passes = []
    wrap_exe = []
    fail_tag = []
    if ("icc" in compiler):
        #add_task(out_name_base, make_run_str, "icc_no_opt", compiler_passes, wrap_exe)
        #fail_tag.append("icc" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "icc_opt", compiler_passes, wrap_exe)
        fail_tag.append("icc" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "icc_knl_opt", compiler_passes, wrap_exe, False)
        fail_tag.append("icc" + os.sep + "knl-runfail")

    if ("gcc" in compiler):
        add_task(out_name_base, make_run_str, "gcc_no_opt", compiler_passes, wrap_exe)
        fail_tag.append("gcc" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "gcc_wsm_opt", compiler_passes, wrap_exe)
        fail_tag.append("gcc" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "gcc_ivb_opt", compiler_passes, wrap_exe)
        fail_tag.append("gcc" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "gcc_bdw_opt", compiler_passes, wrap_exe, False)
        fail_tag.append("gcc" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "gcc_knl_opt", compiler_passes, wrap_exe, False)
        fail_tag.append("gcc" + os.sep + "knl-runfail")
        #add_task(out_name_base, make_run_str, "gcc_skx_opt", compiler_passes, wrap_exe, False)
        #fail_tag.append("gcc" + os.sep + "skx-runfail")

    if ("clang" in compiler):
        add_task(out_name_base, make_run_str, "clang_no_opt", compiler_passes, wrap_exe)
        fail_tag.append("clang" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "clang_opt", compiler_passes, wrap_exe)
        fail_tag.append("clang" + os.sep + "run-uns")
        add_task(out_name_base, make_run_str, "clang_knl_opt", compiler_passes, wrap_exe, False)
        fail_tag.append("clang" + os.sep + "knl-runfail")
        #add_task(out_name_base, make_run_str, "ubsan", compiler_passes, wrap_exe)
        #fail_tag.append("gen")

    return compiler_passes, wrap_exe, fail_tag

def save_test (lock, gen_file, cmd, tag, fail_type, output, seed, compile_cmd = ""):
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
        log.write("Build cmd: " + str(compile_cmd) + "\n")
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
    log.write("Build cmd: " + str(compile_cmd) + "\n")
    log.write("Command: " + str(cmd) + "\n")
    log.write("Error: \n" + str(output) + "\n")
    log.close()
    for i in gen_file.split():
        shutil.copy(i, dest)

test_files_env = " init.cpp driver.cpp check.cpp hash.cpp "
test_files = test_files_env + "func.cpp "
gen_file = test_files + " init.h"

def gen_and_test(num, lock, end_time):
    print ("Job #" + str(num))
    os.chdir(str(num))
    inf = end_time == -1

    compiler_passes = []
    wrap_exe = []
    fail_tag = []
    compiler_passes, wrap_exe, fail_tag = fill_task(args.compiler, num)

    while inf or end_time > time.time():
        seed = str(subprocess.check_output(["bash", "-c", ".." + os.sep + "yarpgen -q"]).decode('UTF-8'))
        print_debug ("Job #" + str(num) + " " + seed, args.verbose)
        pass_res = set()
        tag = ""
        for i in range(len(compiler_passes)):
            ret_code, output, t = run_cmd(num, compiler_passes[i], args.verbose)
            print_debug ("Job #" + str(num) + " (" + seed + ") COMPILE - " + str(compiler_passes[i][2]) + ": " + str(t), args.verbose)
            if (ret_code != 0):
                save_test(lock, gen_file, compiler_passes[i], "", "compfail", output, seed, compiler_passes[i][2])
                continue
            
            ret_code, output, t = run_cmd(num, wrap_exe[i], args.verbose)
            print_debug ("Job #" + str(num) + " (" + seed + ") RUN - " + str(wrap_exe[i]) + ": " + str(t), args.verbose)
            if (ret_code != 0):
                save_test(lock, gen_file, wrap_exe[i], "", "runfail", output, seed, compiler_passes[i][2])
                continue
            else:
                pass_res.add(output)
            if len(pass_res) > 1:
                if tag == "":
                    tag = fail_tag[i]
                print_debug (str(seed) + " " + str(compiler_passes[i]) + " " + str(wrap_exe[i]), args.verbose)
                save_test (lock, gen_file, compiler_passes [i], tag, "runfail", "output differs", seed, compiler_passes[i][2])

def print_compiler_version():
    compilers = args.compiler.split()
    for i in compilers:
        ret_code, output, t = run_cmd(-1, ("which " + i).split (), args.verbose)
        print_debug(i + " folder: " + str(output), args.verbose)
        ret_code, output, t = run_cmd(-1, (i + " -v").split (), args.verbose)
        print_debug(i + " version: " + str(output), args.verbose)

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
