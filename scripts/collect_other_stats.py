#!/usr/bin/python3
###############################################################################
#
# Copyright (c) 2017-2020, Intel Corporation
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
Script for autonomous collect of different statistics from csmith.
"""
###############################################################################

import argparse
import datetime
import logging
import multiprocessing
import os
import shutil
import sys
import time

import common
import gen_test_makefile
import run_gen

# We should init variable, so let's do it this way
script_start_time = datetime.datetime.now()

csmith_timeout = 30

###############################################################################


def form_statistics(stat, prev_len, task_threads=None):
    verbose_stat_str = ""
    for i in gen_test_makefile.CompilerTarget.all_targets:
        if stat.is_stat_collected(i.name):
            verbose_stat_str += "\n=================================\n"
            verbose_stat_str += "Statistics for " + i.name + "\n"
            verbose_stat_str += "Optimization statistics: \n"
            verbose_stat_str += stat.get_stats(i.name, run_gen.StatsVault.opt_stats_id) + "\n\n"
            verbose_stat_str += "Statement statistics: \n"
            verbose_stat_str += stat.get_stats(i.name, run_gen.StatsVault.stmt_stats_id) + "\n"
    verbose_stat_str += "\n=================================\n"

    active = 0
    if task_threads:
        for task in task_threads:
            if task.is_alive():
                active = active + 1

    stat_str = '\r'
    stat_str += "time " + run_gen.strfdelta(datetime.datetime.now() - script_start_time,
                                            "{days} d {hours}:{minutes}:{seconds}") + " | "
    stat_str += " active " + str(active) + " | "
    stat_str += "processed " + str(stat.get_yarpgen_runs(run_gen.ok))

    spaces_needed = prev_len - len(stat_str)
    for i in range(spaces_needed):
        stat_str += " "
    prev_len = len(stat_str)

    return stat_str, verbose_stat_str, prev_len


def print_statistics(stat, task_threads, num_jobs):
    any_alive = True
    prev_len = 0
    while any_alive:
        stat_str, verbose_stat_str, prev_len = form_statistics(stat, prev_len, task_threads)
        common.stat_logger.log(logging.INFO, verbose_stat_str)
        sys.stdout.write(stat_str)
        sys.stdout.flush()

        time.sleep(run_gen.stat_update_delay)

        any_alive = False
        for num in range(num_jobs):
            any_alive |= task_threads[num].is_alive()


def prepare_and_start(work_dir, config_file, timeout, num_jobs, csmith_bin_path, csmith_runtime, csmith_args, optsets):
    common.check_dir_and_create(work_dir)

    # Check for binary of generator
    csmith_home = os.environ.get("CSMITH_HOME")
    if csmith_home is None:
        common.print_and_exit("Please set CSMITH_HOME environment variable")
    csmith_bin = os.path.abspath(csmith_home + os.sep + csmith_bin_path + os.sep + "csmith")
    common.check_and_copy(csmith_bin, work_dir)
    os.chdir(work_dir)
    ret_code, output, err_output, time_expired, elapsed_time = \
        common.run_cmd(["." + os.sep + "csmith", "-v"], csmith_timeout, 0)
    csmith_version_str = str(output, "utf-8")
    # TODO: need to add some check, but I hope that it is safe
    common.log_msg(logging.DEBUG, "Csmith version: " + csmith_version_str)

    common.set_standard("c99")
    gen_test_makefile.set_standard()
    gen_test_makefile.parse_config(config_file)

    compiler_run_args = {}
    optsets_list = optsets.split()
    target_was_found = False
    failed_targets = []
    for i in optsets_list:
        for target in gen_test_makefile.CompilerTarget.all_targets:
            if target.name != i:
                continue
            target_was_found = True
            common.log_msg(logging.DEBUG, "Trying to form compiler args for " + str(i))
            run_str = target.specs.comp_c_name + " "
            run_str += target.args + " "
            if target.arch.comp_name != "":
                run_str += " " + target.specs.arch_prefix + target.arch.comp_name + " "
            run_str += gen_test_makefile.StatisticsOptions.get_options(target.specs) + " "
            run_str += csmith_runtime + " "
            common.log_msg(logging.DEBUG, "Formed compiler args: " + run_str)
            compiler_run_args[i] = run_str

        if not target_was_found:
            failed_targets.append(i)
        target_was_found = False

    if len(failed_targets) > 0:
        common.log_msg(logging.WARNING, "Can't find relevant target: " + str(failed_targets))

    for i in range(num_jobs):
        common.check_dir_and_create(run_gen.process_dir + str(i))

    manager_obj = run_gen.manager()
    stat = manager_obj.Statistics()

    start_time = time.time()
    end_time = start_time + timeout * 60
    if timeout == -1:
        end_time = -1

    task_threads = [0] * num_jobs
    for num in range(num_jobs):
        task_threads[num] = multiprocessing.Process(target=run_csmith,
                                                    args=(num, csmith_args, compiler_run_args, end_time, stat))
        task_threads[num].start()

    print_statistics(stat, task_threads, num_jobs)

    sys.stdout.write("\n")
    for i in range(num_jobs):
        common.log_msg(logging.DEBUG, "Removing " + run_gen.process_dir + str(i) + " dir")
        shutil.rmtree(run_gen.process_dir + str(i))

    stat_str, verbose_stat_str, prev_len = form_statistics(stat, 0)
    sys.stdout.write(verbose_stat_str)
    sys.stdout.flush()


def run_csmith(num, csmith_args, compiler_run_args, end_time, stat):
    common.log_msg(logging.DEBUG, "Job #" + str(num))
    os.chdir(run_gen.process_dir + str(num))
    work_dir = os.getcwd()
    inf = (end_time == -1)

    while inf or end_time > time.time():
        if os.getcwd() != work_dir:
            raise
        common.clean_dir(".")

        ret_code, output, err_output, time_expired, elapsed_time = \
            common.run_cmd([".." + os.sep + "csmith"] + csmith_args.split(), csmith_timeout, num)
        if ret_code != 0:
            continue
        test_file = open("func.c", "w")
        test_file.write(str(output, "utf-8"))
        test_file.close()
        for optset_name, compiler_run_arg in compiler_run_args.items():
            ret_code, output, err_output, time_expired, elapsed_time = \
                common.run_cmd(compiler_run_arg.split() + ["func.c"], run_gen.compiler_timeout, num)
            if ret_code != 0:
                continue

            opt_stats = None
            stmt_stats = None
            if "clang" in optset_name:
                opt_stats = run_gen.StatsParser.parse_clang_opt_stats_file("func.stats")
                stmt_stats = run_gen.StatsParser.parse_clang_stmt_stats_file(str(err_output, "utf-8"))
            stat.add_stats(opt_stats, optset_name, run_gen.StatsVault.opt_stats_id)
            stat.add_stats(stmt_stats, optset_name, run_gen.StatsVault.stmt_stats_id)
            stat.update_yarpgen_runs(run_gen.ok)



###############################################################################

if __name__ == '__main__':
    if os.environ.get("YARPGEN_HOME") is None:
        sys.stderr.write("\nWarning: please set YARPGEN_HOME environment variable to point to yarpgen's directory,"
                         " using " + common.yarpgen_home + " for now\n")

    description = 'Script for autonomous collection of different statistics.'
    parser = argparse.ArgumentParser(description=description, formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--csmith-bin", dest="csmith_bin", default="", type=str,
                        help="Relative path from $CSMITH_HOME to csmith binary")
    parser.add_argument("--csmith-runtime", dest="csmith_runtime", default="", type=str,
                        help="Dependencies, required by csmith")
    parser.add_argument("--csmith-args", dest="csmith_args", default="", type=str,
                        help="Arguments, directly passed to csmith")
    parser.add_argument("--optsets", dest="optsets", default="clang_opt", type=str,
                        help="Targets for testing (see test_sets.txt).")
    parser.add_argument("--work-dir", dest="work_dir", default="collect_stats_dir", type=str,
                        help="Directory, which is used for statistics' collection process.")
    parser.add_argument("-t", "--timeout", dest="timeout", type=int, default=1,
                        help="Timeout for test system in minutes. -1 means infinity")
    parser.add_argument("-j", dest="num_jobs", default=multiprocessing.cpu_count(), type=int,
                        help='Maximum number of instances to run in parallel. By default, it is set to'
                             ' number of processor in your system')
    parser.add_argument("--config-file", dest="config_file",
                        default=os.path.join(common.yarpgen_scripts, gen_test_makefile.default_test_sets_file_name),
                        type=str, help="Configuration file for testing")
    parser.add_argument("--stat-log-file", dest="stat_log_file", default="csmith_statistics.log", type=str,
                        help="Logfile for statistics")
    parser.add_argument("-v", "--verbose", dest="verbose", default=False, action="store_true",
                        help="Increase output verbosity")
    args = parser.parse_args()

    log_level = logging.DEBUG if args.verbose else logging.INFO
    common.setup_logger(None, log_level)

    stat_log_file = common.wrap_log_file(args.stat_log_file, parser.get_default("stat_log_file"))
    common.setup_stat_logger(stat_log_file)

    script_start_time = datetime.datetime.now()
    common.log_msg(logging.DEBUG, "Command line: " + " ".join(str(p) for p in sys.argv))
    common.log_msg(logging.DEBUG, "Start time: " + script_start_time.strftime('%Y/%m/%d %H:%M:%S'))
    common.check_python_version()

    prepare_and_start(work_dir=args.work_dir, timeout=args.timeout, num_jobs=args.num_jobs, config_file=args.config_file,
                      csmith_bin_path=args.csmith_bin, csmith_runtime=args.csmith_runtime, csmith_args=args.csmith_args,
                      optsets=args.optsets)
