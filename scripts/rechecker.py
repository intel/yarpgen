#!/usr/bin/python3
###############################################################################
#
# Copyright (c) 2015-2020, Intel Corporation
# Copyright (c) 2019-2020, University of Utah
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
"""
Script for rechecking of previously found errors
"""
###############################################################################

import argparse
import logging
import multiprocessing
import os
import sys
import queue

import common
import gen_test_makefile
import run_gen
import blame_opt


###############################################################################

def process_dir(directory, task_queue):
    common.log_msg(logging.DEBUG, "Searching for test directories in " + str(directory))
    for root, dirs, files in os.walk(directory):
        for name in dirs:
            if name.startswith("S_"):
                common.log_msg(logging.DEBUG, "Adding " + str(os.path.join(root, name)))
                task_queue.put(os.path.join(root, name))
            else:
                process_dir(os.path.join(root, name), task_queue)
    return task_queue


def prepare_env_and_recheck(input_dir, out_dir, target, num_jobs, config_file):
    if not common.check_if_dir_exists(input_dir):
        common.print_and_exit("Can't use input directory")
    common.check_dir_and_create(out_dir)

    run_gen.gen_test_makefile_and_copy(out_dir, config_file)
    run_gen.dump_testing_sets(target)
    run_gen.print_compilers_version(target)

    lock = multiprocessing.Lock()

    task_queue = multiprocessing.JoinableQueue()
    process_dir(input_dir, task_queue)
    failed_queue = multiprocessing.SimpleQueue()
    passed_queue = multiprocessing.SimpleQueue()

    task_threads = [0] * num_jobs
    for num in range(num_jobs):
        task_threads[num] = \
            multiprocessing.Process(target=recheck,
                                    args=(num, lock, task_queue, failed_queue, passed_queue, target, out_dir))
        task_threads[num].start()

    task_queue.join()
    task_queue.close()

    for num in range(num_jobs):
        task_threads[num].join()


def recheck(num, lock, task_queue, failed_queue, passed_queue, target, out_dir):
    common.log_msg(logging.DEBUG, "Started recheck. Process #" + str(num))
    cwd_save = os.getcwd()
    abs_out_dir = os.path.join(cwd_save, out_dir)
    job_finished = False
    while not job_finished:
        try:
            test_dir = task_queue.get_nowait()
            task_queue.task_done()
            common.log_msg(logging.DEBUG, "#" + str(num) + " test directory: " + str(test_dir))
            abs_test_dir = os.path.join(cwd_save, test_dir)
            common.check_and_copy(os.path.join(os.path.join(cwd_save, out_dir), gen_test_makefile.Test_Makefile_name),
                                  os.path.join(abs_test_dir,               gen_test_makefile.Test_Makefile_name))
            os.chdir(os.path.join(cwd_save, abs_test_dir))

            valid_res = None
            out_res = set()
            prev_out_res_len = 1  # We can't check first result
            for i in gen_test_makefile.CompilerTarget.all_targets:
                if i.specs.name not in target.split():
                    continue

                common.log_msg(logging.DEBUG, "Re-checking target " + i.name)
                ret_code, output, err_output, time_expired, elapsed_time = \
                    common.run_cmd(["make", "-f", gen_test_makefile.Test_Makefile_name, i.name],
                                   run_gen.compiler_timeout, num)
                if time_expired or ret_code != 0:
                    failed_queue.put(test_dir)
                    common.log_msg(logging.DEBUG, "#" + str(num) + " Compilation failed")
                    common.copy_test_to_out(abs_test_dir, os.path.join(abs_out_dir, test_dir), lock)
                    break

                ret_code, output, err_output, time_expired, elapsed_time = \
                    common.run_cmd(["make", "-f", gen_test_makefile.Test_Makefile_name, "run_" + i.name],
                                   run_gen.run_timeout, num)
                if time_expired or ret_code != 0:
                    failed_queue.put(test_dir)
                    common.log_msg(logging.DEBUG, "#" + str(num) + " Execution failed")
                    common.copy_test_to_out(abs_test_dir, os.path.join(abs_out_dir, test_dir), lock)
                    break

                out_res.add(str(output, "utf-8").split()[-1])
                if len(out_res) > prev_out_res_len:
                    prev_out_res_len = len(out_res)
                    failed_queue.put(test_dir)
                    common.log_msg(logging.DEBUG, "#" + str(num) + " Out differs")
                    if not blame_opt.prepare_env_and_blame(abs_test_dir, valid_res, i, abs_out_dir, lock, num):
                        common.copy_test_to_out(abs_test_dir, os.path.join(abs_out_dir, test_dir), lock)
                    break
                valid_res = str(output, "utf-8").split()[-1]

            passed_queue.put(test_dir)
            os.chdir(cwd_save)

        except queue.Empty:
            job_finished = True


###############################################################################

if __name__ == '__main__':
    if os.environ.get("YARPGEN_HOME") is None:
        sys.stderr.write("\nWarning: please set YARPGEN_HOME environment variable to point to yarpgen's directory,"
                         "using " + common.yarpgen_home + " for now\n")

    description = 'Script for rechecking of compiler errors'
    parser = argparse.ArgumentParser(description=description, formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    requiredNamed = parser.add_argument_group('required named arguments')
    requiredNamed.add_argument("-i", "--input-dir", dest="input_dir", type=str, required=True,
                               help="Input directory for re-checking")

    parser.add_argument('--std', dest="std_str", default="c++11", type=str,
                        help='Language standard. Possible variants are ' + str(list(gen_test_makefile.StrToStdId))[1:-1])
    parser.add_argument("-o", "--output-dir", dest="out_dir", default="re-checked", type=str,
                        help="Output directory with relevant fails")
    parser.add_argument("--config-file", dest="config_file",
                        default=os.path.join(common.yarpgen_scripts, gen_test_makefile.default_test_sets_file_name),
                        type=str, help="Configuration file for testing")
    parser.add_argument("--target", dest="target", default="clang ubsan_clang gcc", type=str,
                        help="Targets for testing (see test_sets.txt). By default, possible variants are "
                             "clang, ubsan and gcc (ubsan is a clang with sanitizer options).")
    parser.add_argument("-j", dest="num_jobs", default=multiprocessing.cpu_count(), type=int,
                        help='Maximum number of instances to run in parallel. By default, '
                             'it is set to number of processor in your system')
    parser.add_argument("-v", "--verbose", dest="verbose", default=False, action="store_true",
                        help="Increase output verbosity")
    parser.add_argument("--log-file", dest="log_file", type=str,
                        help="Logfile")
    args = parser.parse_args()

    log_level = logging.DEBUG if args.verbose else logging.INFO
    common.setup_logger(args.log_file, log_level)

    common.check_python_version()
    common.set_standard(args.std_str)
    gen_test_makefile.set_standard()
    prepare_env_and_recheck(args.input_dir, args.out_dir, args.target, args.num_jobs, args.config_file)
