#!/usr/bin/python3.3
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
import logging
import multiprocessing
import multiprocessing.managers
import os
import shutil
import subprocess
import sys
import time

import common
import gen_test_makefile

res_dir = "result"
process_dir = "process_"
Test_Makefile_name = "Test_Makefile"

yarpgen_timeout = 60
compiler_timeout = 600
run_timeout = 300 
stat_update_delay = 10
stat_verbose_delay = 60

script_start_time = datetime.datetime.now() # We should init variable, so let's do it this way

###############################################################################

class MyManager(multiprocessing.managers.BaseManager): pass

def Manager():
    m = MyManager()
    m.start()
    return m 


total = "total"
ok = "ok"
runfail = "runfail"
runfail_timeout = "runfail_timeout"
compfail = "compfail"
compfail_timeout = "compfail_timeout"
out_dif = "different_output"

class CmdRun (object):

    def __init__ (self, name):
        self.name = name
        self.total = 0
        self.ok = 0
        self.compfail = 0
        self.compfail_timeout = 0
        self.runfail = 0
        self.runfail_timeout = 0
        self.out_dif = 0
        self.duration = datetime.timedelta(0)

    def update (self, tag):
        self.total += 1
        if (tag == ok):
            self.ok += 1
        if (tag == runfail):
            self.runfail += 1
        if (tag == runfail_timeout):
            self.runfail_timeout += 1
        if (tag == compfail):
            self.compfail += 1
        if (tag == compfail_timeout):
            self.compfail_timeout += 1
        if (tag == out_dif):
            self.out_dif += 1

    def get_value (self, tag):
        if (tag == total):
            return self.total
        if (tag == ok):
            return self.ok
        if (tag == runfail):
            return self.runfail
        if (tag == runfail_timeout):
            return self.runfail_timeout
        if (tag == compfail):
            return self.compfail
        if (tag == compfail_timeout):
            return self.compfail_timeout
        if (tag == out_dif):
            return self.out_dif

    def update_duration(self, interval):
        self.duration += interval

    def get_duration (self):
        return self.duration

    def get_name (self):
        return name



class Statistics (object):
    def __init__(self):
        self.yarpgen_runs = CmdRun("yarpgen")
        self.target_runs = {} 
        #TODO: we create objects for every target, but we can choose less in arguments
        for i in gen_test_makefile.Compiler_target.all_targets:
            self.target_runs[i.name] = CmdRun(i.name)

    def update_yarpgen_runs(self, tag):
        self.yarpgen_runs.update(tag)

    def get_yarpgen_runs(self, tag):
        return self.yarpgen_runs.get_value(tag)

    def update_yarpgen_duration(self, interval):
        self.yarpgen_runs.update_duration(interval)

    def get_yarpgen_duration(self):
        return self.yarpgen_runs.get_duration()

    def update_target_runs(self, target_name, tag):
        self.target_runs[target_name].update(tag)

    def get_target_runs(self, target_name, tag):
        return self.target_runs[target_name].get_value(tag)

    def update_target_duration(self, target_name, interval):
        self.target_runs[target_name].update_duration(interval)

    def get_target_duration(self, target_name):
        return self.target_runs[target_name].get_duration()

MyManager.register("Statistics", Statistics)



def prepare_env_and_start_testing (verbose, out_dir, timeout, compiler, num_jobs, stat_verbose, config_file):
    common.check_dir_and_create (out_dir)

    # Check for binary of generator
    yarpgen_bin = os.path.abspath(common.yarpgen_home + os.sep + "yarpgen")
    common.check_and_copy (yarpgen_bin, out_dir)
    ret_code, output, err_output, time_expired = common.run_cmd([yarpgen_bin, "-v"], yarpgen_timeout, 0)
    common.yarpgen_version = output
    #TODO: need to add some check, but I hope that it is safe
    common.log_msg(logging.DEBUG, "YARPGEN version: " + str(common.yarpgen_version))

    # Generate Test_Makefile and copy it
    Test_Makefile_location = os.path.abspath(common.yarpgen_home + os.sep + Test_Makefile_name)
    gen_test_makefile.gen_makefile(Test_Makefile_location, True, verbose, config_file)
    common.check_and_copy (Test_Makefile_location, out_dir)

    common.log_msg(logging.INFO, "Testing sets: ")
    for i in gen_test_makefile.Compiler_target.all_targets:
        if (i.specs.name in compiler.split()):
            common.log_msg(logging.INFO, i.name)

    # Search for target compilers and print their location and version
    for i in compiler.split():
        comp_exec_name = gen_test_makefile.Compiler_specs.all_comp_specs [i].comp_name
        if not common.if_exec_exist(comp_exec_name):
            common.print_and_exit("Can't find " + comp_exec_name + " binary")
        ret_code, output, err_output, time_expired = common.run_cmd([comp_exec_name, "--version"])
        #TODO: I hope it will work for all compilers
        common.log_msg(logging.DEBUG, str(output.splitlines() [0], "utf-8"))
        gen_test_makefile.Compiler_specs.all_comp_specs [i].set_version(str(output.splitlines() [0], "utf-8"))

    os.chdir(out_dir)
    common.check_dir_and_create(res_dir)
    for i in range(num_jobs):
        common.check_dir_and_create(process_dir + str(i))
        common.check_and_copy (Test_Makefile_location, process_dir + str(i))

    lock = multiprocessing.Lock()
    manager = Manager()
    stat = manager.Statistics()

    start_time = time.time()
    end_time = start_time + timeout * 10
    if timeout == -1:
        end_time = -1

    task_threads = [0] * num_jobs
    for num in range(num_jobs):
        task_threads [num] =  multiprocessing.Process(target = gen_and_test, args = (num, lock, end_time, stat, compiler))
        task_threads [num].start()

    any_alive = True
    stat_str = ""
    while any_alive:
        lock.acquire()
        if (stat_str != ""):
            for i in range(stat_str.count("\n")):
                sys.stdout.write("\b\r")
        sys.stdout.flush()
        stat_str = ""
        stat_str += "\n##########################\n"
        stat_str += "YARPGEN runs stat:\n"
        stat_str += "Time: " + datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S') + "\n"
        stat_str += "duration: " + str(datetime.datetime.now() - script_start_time) + "\n"

        if stat_verbose:
            stat_str += "\tcpu time: " + str(stat.get_yarpgen_duration()) + "\n"
            stat_str += "\t" + total + " : " + str(stat.get_yarpgen_runs(total)) + "\n"
            stat_str += "\t" + ok + " : " + str(stat.get_yarpgen_runs(ok)) + "\n"
            stat_str += "\t" + runfail_timeout + " : " + str(stat.get_yarpgen_runs(runfail_timeout)) + "\n"
            stat_str += "\t" + runfail + " : " + str(stat.get_yarpgen_runs(runfail)) + "\n"
            for i in gen_test_makefile.Compiler_target.all_targets:
                if (not i.specs.name in compiler.split()):
                    continue
                stat_str += "\n##########################\n"
                stat_str += i.name + " stat:" + "\n"
                stat_str += "\tcpu time: " + str(stat.get_target_duration(i.name))  + "\n"
                stat_str += "\t" + total + " : " + str(stat.get_target_runs(i.name, total)) + "\n"
                stat_str += "\t" + ok + " : " + str(stat.get_target_runs(i.name, ok)) + "\n"
                stat_str += "\t" + runfail_timeout + " : " + str(stat.get_target_runs(i.name, runfail_timeout)) + "\n"
                stat_str += "\t" + runfail + " : " + str(stat.get_target_runs(i.name, runfail)) + "\n"
                stat_str += "\t" + compfail_timeout + " : " + str(stat.get_target_runs(i.name, compfail_timeout)) + "\n"
                stat_str += "\t" + compfail + " : " + str(stat.get_target_runs(i.name, compfail)) + "\n"
                stat_str += "\t" + out_dif + " : " + str(stat.get_target_runs(i.name, out_dif)) + "\n"
        else:
            total_duration = stat.get_yarpgen_duration()
            total_gen_errors = stat.get_yarpgen_runs(runfail_timeout)
            total_gen_errors += stat.get_yarpgen_runs(runfail)
            stat_str += "total yarpgen errors: " + str(total_gen_errors) + "\n"
            total_runs = 0
            total_ok = 0
            total_runfail_timeout = 0
            total_runfail = 0
            total_compfail_timeout = 0
            total_compfail = 0
            total_out_dif = 0
            for i in gen_test_makefile.Compiler_target.all_targets:
                if (not i.specs.name in compiler.split()):
                    continue
                total_duration += stat.get_target_duration(i.name)
                total_runs += stat.get_target_runs(i.name, total)
                total_ok += stat.get_target_runs(i.name, ok)
                total_runfail_timeout += stat.get_target_runs(i.name, runfail_timeout)
                total_runfail += stat.get_target_runs(i.name, runfail)
                total_compfail_timeout += stat.get_target_runs(i.name, compfail_timeout)
                total_compfail += stat.get_target_runs(i.name, compfail)
                total_out_dif += stat.get_target_runs(i.name, out_dif)
            stat_str += "total cpu time: " + str(total_duration) + "\n"
            stat_str += "total target runs: " + str(total_runs) + "\n"
            stat_str += "total ok: " + str(total_ok) + "\n"
            stat_str += "total runfail timeout: " + str(total_runfail_timeout) + "\n"
            stat_str += "total runfail: " + str(total_runfail) + "\n"
            stat_str += "total compfail timeout: " + str(total_compfail_timeout) + "\n"
            stat_str += "total compfail: " + str(total_compfail) + "\n"
            stat_str += "total out_dif: " + str(total_out_dif) + "\n"

        common.stat_logger.info(stat_str)
        sys.stdout.write(stat_str)
        sys.stdout.flush()
        lock.release()

        any_alive = task_threads[num_jobs - 1].is_alive()
        for num in range(num_jobs - 1):
            any_alive |= task_threads [num].is_alive()
        if (stat_verbose):
            time.sleep(stat_verbose_delay)
        else:
            time.sleep(stat_update_delay)

    sys.stdout.write("\n")
    for i in range(num_jobs):
        common.log_msg(logging.DEBUG, "Removing " + process_dir + str(i) + " dir")
        shutil.rmtree(process_dir + str(i))



def gen_and_test(num, lock, end_time, stat, compilers):
    common.log_msg(logging.DEBUG,"Job #" + str(num))
    os.chdir(process_dir + str(num))
    inf = (end_time == -1)

    while inf or end_time > time.time():
        seed = ""
        common.remove_file_if_exists(gen_test_makefile.time_log_file_name)
        #TODO: maybe, it is better to call generator through Makefile?
        yarpgen_run_list = [gen_test_makefile.time_exec] + gen_test_makefile.time_args + [".." + os.sep + "yarpgen", "-q"]
        ret_code, output, err_output, time_expired = common.run_cmd(yarpgen_run_list, yarpgen_timeout, num)
        seed = str(output, "utf-8").split()[1][:-2]
        if (time_expired):
            common.log_msg(logging.WARNING, "Generator has failed (" + runfail_timeout + ")")
            stat.update_yarpgen_runs(runfail_timeout)
            save_test (lock, num, seed, output, err_output, None, runfail_timeout)
            continue
        if (ret_code != 0):
            common.log_msg(logging.WARNING, "Generator has failed (" + runfail + ")")
            stat.update_yarpgen_runs(runfail)
            save_test (lock, num, seed, output, err_output, None, runfail)
            continue
        stat.update_yarpgen_runs(ok)
        stat.update_yarpgen_duration(common.parse_time_log(gen_test_makefile.time_log_file_name))
        out_res = set()
        prev_out_res_len = 1 # We can't check first result
        for i in gen_test_makefile.Compiler_target.all_targets:
            if (not i.specs.name in compilers.split()):
                continue
            # Clear time_log file in case it has left after previous target
            common.remove_file_if_exists(gen_test_makefile.time_log_file_name)
            lock.acquire()
            common.log_msg(logging.DEBUG, "From process #" + str(num) + ": " + str(output, "utf-8"))
            lock.release()

            ret_code, output, err_output, time_expired = common.run_cmd(["make", "-f", Test_Makefile_name, i.name], compiler_timeout, num)
            if (time_expired):
                common.log_msg(logging.WARNING, "Task " + i.name + " has failed (" + compfail_timeout + ")")
                stat.update_target_runs(i.name, compfail_timeout)
                save_test (lock, num, seed, output, err_output, i, compfail_timeout)
                continue
            if (ret_code != 0):
                common.log_msg(logging.WARNING, "Task " + i.name + " has failed (" + compfail + ")")
                stat.update_target_runs(i.name, compfail)
                save_test (lock, num, seed, output, err_output, i, compfail)
                continue

            ret_code, output, err_output, time_expired = common.run_cmd(["make", "-f", Test_Makefile_name, "run_" + i.name], run_timeout, num)
            if (time_expired):
                common.log_msg(logging.WARNING, "Run of " + i.name + " has failed (" + runfail_timeout + ")")
                stat.update_target_runs(i.name, runfail_timeout)
                save_test (lock, num, seed, output, err_output, i, runfail_timeout)
                continue
            if (ret_code != 0):
                common.log_msg(logging.WARNING, "Run of " + i.name + " has failed ( " + runfail + ")")
                stat.update_target_runs(i.name, runfail)
                save_test (lock, num, seed, output, err_output, i, runfail)
                continue

            stat.update_target_duration(i.name, common.parse_time_log(gen_test_makefile.time_log_file_name))

            out_res.add(str(output, "utf-8").split()[-1])
            if (len(out_res) > prev_out_res_len):
                prev_out_res_len = len(out_res) 
                common.log_msg(logging.WARNING, "Output for " + i.name + " differs")
                stat.update_target_runs(i.name, out_dif)
                save_test (lock, num, seed, output, err_output, i, "output")
            stat.update_target_runs(i.name, ok) 


def save_test (lock, num, seed, output, err_output, target, fail_tag):
    dest = ".." + os.sep + res_dir
    # Check and/or create compilers codename dir
    if (target != None):
        dest += os.sep + target.specs.name
    else:
        dest += os.sep + "gen_fail"
    lock.acquire()
    common.check_dir_and_create(dest)
    # Check and/or create fail_tag dir
    dest += os.sep + str(fail_tag)
    common.check_dir_and_create(dest)
    if (target != None and target.arch.sde_arch.name != ""):
        dest += os.sep + target.arch.sde_arch.name
        common.check_dir_and_create(dest)
    dest += os.sep + "S_" + seed
    if (os.path.exists(dest)):
        if (not os.path.isdir(dest)):
            common.print_and_exit ("Can't use '" + dest + "' directory")
            lock.release()
            return
    else:
        os.makedirs(dest)
    log = open(dest + os.sep + "log.txt", "a")

    log.write("YARPGEN version: " + str(common.yarpgen_version) + "\n")
    log.write("Seed: " + str(seed) + "\n")
    log.write("Time: " + datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S') + "\n")
    log.write("Type: " + str(fail_tag) + "\n")
    #If it is generator's error, we can't copy test's source files
    if (target == None):
        log.close()
        shutil.copy(".." + os.sep + "yarpgen", dest)
        lock.release()
        return
    log.write("Target: " + str(target.name) + "\n")
    log.write("Compiler version: " + str(target.specs.version) + "\n")
    log.write("Output: \n" + str(output, "utf-8") + "\n\n")
    log.write("Err_output:\n" + str(err_output, "utf-8") + "\n")
    log.write("====================================\n")
    log.close()

    test_files = gen_test_makefile.sources.value.split() + gen_test_makefile.headers.value.split()
    test_files.append(Test_Makefile_name)
    for i in (test_files): 
        common.check_and_copy(i, dest)
    lock.release()
   


###############################################################################

class CustomFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawDescriptionHelpFormatter):
    pass

if __name__ == '__main__':
    if os.environ.get("YARPGEN_HOME") == None:
       sys.stderr.write("\nWarning: please set YARPGEN_HOME envirnoment variable to point to test generator path, using " + common.yarpgen_home + " for now\n")

    description = "The startup script for compiler's testing system."
    epilog = '''
Examples:
Run testing of gcc and clang with clang sanitizer forever
        run_gen.py -c "gcc clang ubsan" -t -1
Run testing with debug logging level; save log to specified file
        run_gen.py -v --log-file my_log_file.txt
Run testing with verbose statistics and save it to specified file
        run_gen.py -sv --stat-log-file my_stat_log_file.txt
Use specified folder for testing
        run_gen.py -o my_folder
    '''
    parser = argparse.ArgumentParser(description = description, epilog = epilog, formatter_class = CustomFormatter)
    parser.add_argument("-o", "--output", dest = "out_dir", default = "testing", type = str,
                        help = "Directory, which is used for testing.")
    parser.add_argument("-t", "--timeout", dest = "timeout", type = int, default = 1,
                        help = "Timeout for testing system in hours. -1 means infinity")
    parser.add_argument("-c", "--compiler", dest = "compiler", default = "clang ubsan gcc", type = str,
                        help = "Compilers for testing. Possible variants are clang, ubsan and gcc.")
    parser.add_argument("-j", dest = "num_jobs", default = multiprocessing.cpu_count(), type = int,
                        help='Maximum number of instances to run in parallel')
    parser.add_argument("--config-file", dest = "config_file", default = "test_sets.txt", type = str,
                            help = "Configuration file for testing")
    default_log_file = "run_gen_log"
    parser.add_argument("--log-file", dest="log_file", default = default_log_file, type = str,
                        help = "Logfile")
    parser.add_argument("-v", "--verbose", dest = "verbose", default = False, action = "store_true",
                        help = "Increase output verbosity")
    parser.add_argument("-sv", "--stat-verbose", dest = "stat_verbose", default = False, action = "store_true",
                        help = "Increase output verbosity for statistics")
    default_stat_log_file = "statistics_log"
    parser.add_argument("--stat-log-file", dest="stat_log_file", default = default_stat_log_file, type = str,
                        help = "Logfile")
    args = parser.parse_args()

    log_level = logging.DEBUG if (args.verbose) else logging.INFO
    common.setup_logger(logger_name = common.stderr_logger_name, log_level = log_level, write_to_stderr = True)

    logs_to_dir = "."
    log_file_is_def = str(args.log_file) == default_log_file
    stat_log_file_is_def = str(args.stat_log_file) == default_stat_log_file
    if (log_file_is_def or stat_log_file_is_def):
        logs_to_dir = "testing_log" + "_" + datetime.datetime.now().strftime('%Y_%m_%d_%H_%M_%S')
        common.check_dir_and_create(logs_to_dir)

    log_file = str(args.log_file) if not log_file_is_def else (logs_to_dir + os.sep + str(args.log_file))
    common.setup_logger(logger_name = common.file_logger_name, log_file = log_file, log_level = logging.INFO)

    stat_log_file = str(args.stat_log_file) if not stat_log_file_is_def else (logs_to_dir + os.sep + str(args.stat_log_file))
    common.setup_logger(logger_name = common.stat_logger_name, log_file = stat_log_file, file_mode = "w", log_level = logging.INFO)

    script_start_time = datetime.datetime.now()
    common.log_msg(logging.DEBUG, "Start time: " + script_start_time.strftime('%Y/%m/%d %H:%M:%S'))
    common.check_python_version()
    prepare_env_and_start_testing(args.verbose, os.path.abspath(args.out_dir), args.timeout, args.compiler, args.num_jobs, args.stat_verbose, args.config_file)
