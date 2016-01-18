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

def reduce():
    compiler_passes = []
    wrap_exe = []
    fail_tag = []
    compiler_passes, wrap_exe, fail_tag = run_gen.fill_task(args.compiler)

    seed = subprocess.check_output(["bash", "-c", ".." + os.sep + "yarpgen -q -s " + args.seed])

    pass_res = set()
    err_exist = False
    for i in range(len(compiler_passes)):
        ret_code, output = run_gen.run_cmd("0", compiler_passes[i], args.verbose)
        run_gen.print_debug("compile output: " + str(output), args.verbose)
        if (ret_code != 0):
            err_exist = True
            break
        ret_code, output = run_gen.run_cmd("0", wrap_exe [i], args.verbose)
        run_gen.print_debug("run output: " + str(output), args.verbose)
        if (ret_code != 0):
            err_exist = True
            break
        else:
            pass_res.add(output)
        if len(pass_res) > 1:
            err_exist = True
            break
    with open("reduce_log.txt", "a") as red_log:
        red_log.write("\nResult\n")
        red_log.write(str(int(err_exist)))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script for auto-reducing')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='Enable verbose mode.')
    parser.add_argument('-f', '--folder', dest='folder', type=str, default='.'+os.sep,
                        help='Work folder')
    parser.add_argument('-c', '--compiler', dest='compiler', default="icc clang", type=str,
                        help='Test compilers')
    parser.add_argument('-s', '--seed', dest ='seed', type=str, default='0',
                        help='Seed for yarpgen')
    args = parser.parse_args()

    print("Working folder is " + args.folder)

    os.chdir(args.folder)
    shutil.copy(yarpgen_home + os.sep + "yarpgen", ".")
    
    reduce()
