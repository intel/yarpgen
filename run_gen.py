#!/usr/bin/python3
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
"""
Script for autonomous testing with YARP Generator
"""
###############################################################################

import argparse
import datetime
import logging
import multiprocessing
import multiprocessing.managers
import os
import shutil
import sys
import time
import queue

import common
import gen_test_makefile

res_dir = "result"
process_dir = "process_"

yarpgen_timeout = 60
compiler_timeout = 600
run_timeout = 300
stat_update_delay = 10

script_start_time = datetime.datetime.now()  # We should init variable, so let's do it this way

###############################################################################


class MyManager(multiprocessing.managers.BaseManager):
    pass


def manager():
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

# class representing the test
class Test(object):
    # list of files
    # seed, string
    # generator's logs
    # generation time
    # return code
    # is time_expired
    # path
    # process number for logging

    # status values
    STATUS_ok=1
    STATUS_fail=2
    STATUS_fail_timeout=3

    # Generate new test
    # stat is statistics object
    # seed is optional, if we want to generate some particular seed.
    # proc_num is optinal debug info to track in what process we are running this activity.
    def __init__(self, stat, seed="", proc_num=-1):
        # Run generator
        yarpgen_run_list = [".." + os.sep + "yarpgen", "-q"]
        if seed:
            yarpgen_run_list += ["-s", seed]
        self.ret_code, self.stdout, self.stderr, self.is_time_expired, self.elapsed_time = \
            common.run_cmd(yarpgen_run_list, yarpgen_timeout, proc_num)

        # Files that belongs to generate test. They are hardcoded for now.
        # Generator may report them in output later and we may need to parse it.
        self.files = ["check.cpp", "driver.cpp", "func.cpp", "hash.cpp", "init.cpp", "init.h"]

        # Parse generated seed.
        if not seed:
            if self.stdout:
                seed = str(self.stdout, "utf-8").split()[1][:-2]
            else:
                seed = str(proc_num) + "_" + datetime.datetime.now().strftime('%Y_%m_%d_%H_%M_%S')
        self.seed = seed

        self.path = os.getcwd()
        self.proc_num = proc_num
        self.stat = stat

        # Update statistics and set the status
        stat.update_yarpgen_duration(datetime.timedelta(seconds=self.elapsed_time))
        if self.is_time_expired:
            common.log_msg(logging.WARNING, "Generator has failed (" + runfail_timeout + ")")
            self.status = self.STATUS_fail_timeout
            stat.update_yarpgen_runs(runfail)
        elif self.ret_code != 0:
            common.log_msg(logging.WARNING, "Generator has failed (" + runfail + ")")
            self.status = self.STATUS_fail
            stat.update_yarpgen_runs(runfail)
        else:
            self.status = self.STATUS_ok
            stat.update_yarpgen_runs(ok)

        # Initialize set of test runs
        self.successful_test_runs = []

    # Check status
    def is_ok(self):
        return self.status == self.STATUS_ok

    # Save test
    def save(self, lock):
        # TODO: add list of files
        if self.status == self.STATUS_fail_timeout:
            save_status = runfail_timeout
        elif self.status == self.STATUS_fail:
            save_status = runfail
        else:
            save_status = ok
        save_test(lock, self.proc_num, self.seed, self.stdout, self.stderr, None, save_status)

    # Add successful test run
    def add_success_run(self, test_run):
        self.successful_test_runs.append(test_run)

    # Verify the results and if bad results are found, report / save them.
    def verify_results(self, lock):
        results = set()
        for t in self.successful_test_runs:
            assert t.status == TestRun.STATUS_ok
            len1 = len(results)
            results.add(t.checksum)
            len2 = len(results)
            if len1 > 0 and len2 > len1:
                self.stat.update_target_runs(t.optset, out_dif)
                save_test(lock, self.proc_num, self.seed, self.stdout, self.stderr, t.target, "output")

# End of Test class

# class representing the result of running a single opt-set for the test
class TestRun(object):
    # opt-set
    # build / execution log
    # build / execution time
    # run result: pass / compfail / compfail_timeout / runfail / runfail_timeout / miscompare.
    # compilation log

    STATUS_ok = 1
    STATUS_miscompare = 2
    STATUS_compfail = 3
    STATUS_compfail_timeout = 4
    STATUS_runfail = 5
    STATUS_runfail_timeout = 6
    STATUS_not_built = 7
    STATUS_not_run = 8

    def __init__(self, stat, target, seed, proc_num=-1):
        self.stat = stat
        self.target = target
        self.optset = target.name
        self.seed = seed
        self.proc_num = proc_num
        self.status = self.STATUS_not_built
        self.files = []

    # Build test
    def build(self):
        # build
        self.build_ret_code, self.build_stdout, self.build_stderr, self.is_build_time_expired, self.build_elapsed_time = \
            common.run_cmd(["make", "-f", gen_test_makefile.Test_Makefile_name, self.optset], compiler_timeout, self.proc_num)
        # update status and stats
        if self.is_build_time_expired:
            self.stat.update_target_runs(self.optset, compfail_timeout)
            self.status = self.STATUS_compfail_timeout
        elif self.build_ret_code != 0:
            self.stat.update_target_runs(self.optset, compfail)
            self.status = self.STATUS_compfail
        else:
            self.status = self.STATUS_not_run
        # update file list
        expected_files = ["init.o", "driver.o", "func.o", "check.o", "hash.o", self.optset+"_out"]
        for f in expected_files:
            if os.path.isfile(f):
                self.files.append(f)
        return self.status == self.STATUS_not_run

    # Run test
    def run(self):
        # run
        self.run_ret_code, self.run_stdout, self.run_stderr, self.run_is_time_expired, self.run_elapsed_time = \
            common.run_cmd(["make", "-f", gen_test_makefile.Test_Makefile_name, "run_" + self.optset], run_timeout, self.proc_num)
        # update status and stats
        if self.run_is_time_expired:
            self.stat.update_target_runs(self.optset, runfail_timeout)
            self.status = self.STATUS_runfail_timeout
        elif self.run_ret_code != 0:
            self.stat.update_target_runs(self.optset, runfail)
            self.status = self.STATUS_runfail
        else:
            self.stat.update_target_runs(self.optset, ok)
            self.status = self.STATUS_ok
            self.checksum = str(self.run_stdout, "utf-8").split()[-1]
        self.stat.update_target_duration(self.optset, datetime.timedelta(seconds=self.build_elapsed_time+self.run_elapsed_time))
        return self.status == self.STATUS_ok

    def save(self, lock):
        assert self.status != self.STATUS_not_built
        # TODO: add list of files
        if self.status == self.STATUS_compfail_timeout:
            save_status = compfail_timeout
            out = self.build_stdout
            err = self.build_stderr
        elif self.status == self.STATUS_compfail:
            save_status = compfail
            out = self.build_stdout
            err = self.build_stderr
        elif self.status == self.STATUS_runfail_timeout:
            save_status = runfail_timeout
            out = self.run_stdout
            err = self.run_stderr
        elif self.status == self.STATUS_runfail:
            save_status = runfail
            out = self.run_stdout
            err = self.run_stderr
        elif self.status == self.STATUS_miscompare:
            save_status = "output"
            out = self.run_stdout
            err = self.run_stderr
        elif self.status == self.STATUS_ok:
            return
        else:
            raise

        save_test(lock, self.proc_num, self.seed, out, err, self.target, save_status)
# End of TestRun class


class CmdRun (object):

    def __init__(self, name):
        self.name = name
        self.total = 0
        self.ok = 0
        self.compfail = 0
        self.compfail_timeout = 0
        self.runfail = 0
        self.runfail_timeout = 0
        self.out_dif = 0
        self.duration = datetime.timedelta(0)

    def update(self, tag):
        self.total += 1
        global ok
        if tag == ok:
            self.ok += 1
        global runfail
        if tag == runfail:
            self.runfail += 1
        global runfail_timeout
        if tag == runfail_timeout:
            self.runfail_timeout += 1
        global compfail
        if tag == compfail:
            self.compfail += 1
        global compfail_timeout
        if tag == compfail_timeout:
            self.compfail_timeout += 1
        global out_dif
        if tag == out_dif:
            self.out_dif += 1

    def get_value(self, tag):
        if tag == total:
            return self.total
        if tag == ok:
            return self.ok
        if tag == runfail:
            return self.runfail
        if tag == runfail_timeout:
            return self.runfail_timeout
        if tag == compfail:
            return self.compfail
        if tag == compfail_timeout:
            return self.compfail_timeout
        if tag == out_dif:
            return self.out_dif

    def update_duration(self, interval):
        self.duration += interval

    def get_duration(self):
        return self.duration

    def get_name(self):
        return self.name


class Statistics (object):
    def __init__(self):
        self.yarpgen_runs = CmdRun("yarpgen")
        self.target_runs = {}
        # TODO: we create objects for every target, but we can choose less in arguments
        for i in gen_test_makefile.CompilerTarget.all_targets:
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
        if tag != ok:
            common.log_msg(logging.DEBUG, "Run of " + target_name + " has failed (" + tag + ")")
        self.target_runs[target_name].update(tag)

    def get_target_runs(self, target_name, tag):
        return self.target_runs[target_name].get_value(tag)

    def update_target_duration(self, target_name, interval):
        self.target_runs[target_name].update_duration(interval)

    def get_target_duration(self, target_name):
        return self.target_runs[target_name].get_duration()

MyManager.register("Statistics", Statistics)


def strfdelta(time_delta, format_str):
    time_dict = {"days": time_delta.days}
    time_dict["hours"], rem = divmod(time_delta.seconds, 3600)
    time_dict["minutes"], time_dict["seconds"] = divmod(rem, 60)
    return format_str.format(**time_dict)


def get_testing_speed(seed_num, time_delta):
    minutes = time_delta.total_seconds() / 60
    return "{:.2f}".format(seed_num / minutes) + " seed/min"


def form_statistics(stat, targets, prev_len):
    verbose_stat_str = ""

    testing_speed = get_testing_speed(stat.get_yarpgen_runs(total), datetime.datetime.now() - script_start_time)

    # TODO: make this section smaller
    verbose_stat_str += "\n##########################\n"
    verbose_stat_str += "YARPGEN runs stat:\n"
    verbose_stat_str += "Time: " + datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S') + "\n"
    verbose_stat_str += "duration: " + strfdelta(datetime.datetime.now() - script_start_time,
                                                 "{days} d {hours}:{minutes}:{seconds}") + "\n"
    verbose_stat_str += "testing speed: " + testing_speed + "\n"
    verbose_stat_str += "\n##########################\n"
    verbose_stat_str += "generator stat:" + "\n"
    verbose_stat_str += "cpu time: " + strfdelta(stat.get_yarpgen_duration(),
                                                 "{days} d {hours}:{minutes}:{seconds}") + "\n"
    verbose_stat_str += "\t" + total + " : " + str(stat.get_yarpgen_runs(total)) + "\n"
    verbose_stat_str += "\t" + ok + " : " + str(stat.get_yarpgen_runs(ok)) + "\n"
    verbose_stat_str += "\t" + runfail_timeout + " : " + str(stat.get_yarpgen_runs(runfail_timeout)) + "\n"
    verbose_stat_str += "\t" + runfail + " : " + str(stat.get_yarpgen_runs(runfail)) + "\n"

    total_cpu_duration = stat.get_yarpgen_duration()
    total_gen_errors = stat.get_yarpgen_runs(runfail_timeout)
    total_gen_errors += stat.get_yarpgen_runs(runfail)
    total_seeds = stat.get_yarpgen_runs(total)
    total_runs = 0
    total_ok = 0
    total_runfail_timeout = 0
    total_runfail = 0
    total_compfail_timeout = 0
    total_compfail = 0
    total_out_dif = 0

    for i in gen_test_makefile.CompilerTarget.all_targets:
        if i.specs.name not in targets.split():
            continue
        verbose_stat_str += "\n##########################\n"
        verbose_stat_str += i.name + " stat:" + "\n"
        verbose_stat_str += "\tcpu time: " + strfdelta(stat.get_target_duration(i.name),
                                                       "{days} d {hours}:{minutes}:{seconds}") + "\n"
        total_cpu_duration += stat.get_target_duration(i.name)
        verbose_stat_str += "\t" + total + " : " + str(stat.get_target_runs(i.name, total)) + "\n"
        total_runs += stat.get_target_runs(i.name, total)
        verbose_stat_str += "\t" + ok + " : " + str(stat.get_target_runs(i.name, ok)) + "\n"
        verbose_stat_str += "\t" + compfail_timeout + " : " + str(stat.get_target_runs(i.name, compfail_timeout)) + "\n"
        total_compfail_timeout += stat.get_target_runs(i.name, compfail_timeout)
        verbose_stat_str += "\t" + compfail + " : " + str(stat.get_target_runs(i.name, compfail)) + "\n"
        total_compfail += stat.get_target_runs(i.name, compfail)
        total_ok += stat.get_target_runs(i.name, ok)
        verbose_stat_str += "\t" + runfail_timeout + " : " + str(stat.get_target_runs(i.name, runfail_timeout)) + "\n"
        total_runfail_timeout += stat.get_target_runs(i.name, runfail_timeout)
        verbose_stat_str += "\t" + runfail + " : " + str(stat.get_target_runs(i.name, runfail)) + "\n"
        total_runfail += stat.get_target_runs(i.name, runfail)
        verbose_stat_str += "\t" + out_dif + " : " + str(stat.get_target_runs(i.name, out_dif)) + "\n"
        total_out_dif += stat.get_target_runs(i.name, out_dif)

    stat_str = '\r'
    stat_str += "time " + strfdelta(datetime.datetime.now() - script_start_time,
                                    "{days} d {hours}:{minutes}:{seconds}") + " | "
    stat_str += "cpu time: " + strfdelta(total_cpu_duration, "{days} d {hours}:{minutes}:{seconds}") + " | "
    stat_str += testing_speed + " | "
    stat_str += "seeds/targets: " + str(total_seeds)+"/"+str(total_runs) + " | "
    stat_str += "Errors(g/ct/c/rt/r/d): " + str(total_gen_errors) + "/"
    stat_str += str(total_compfail_timeout) + "/"
    stat_str += str(total_compfail) + "/"
    stat_str += str(total_runfail_timeout) + "/"
    stat_str += str(total_runfail) + "/"
    stat_str += str(total_out_dif)

    spaces_needed = prev_len - len(stat_str)
    for i in range(spaces_needed):
        stat_str += " "
    prev_len = len(stat_str)
    return stat_str, verbose_stat_str, prev_len


def print_online_statistics(lock, stat, targets, task_threads, num_jobs):
    any_alive = True
    prev_len = 0
    while any_alive:
        lock.acquire()
        stat_str, verbose_stat_str, prev_len = form_statistics(stat, targets, prev_len)
        common.stat_logger.log(logging.INFO, verbose_stat_str)
        sys.stdout.write(stat_str)
        sys.stdout.flush()
        lock.release()

        any_alive = False
        for num in range(num_jobs):
            any_alive |= task_threads[num].is_alive()

        time.sleep(stat_update_delay)


def gen_test_makefile_and_copy(dest, config_file):
    test_makefile_location = os.path.abspath(common.yarpgen_home + os.sep + gen_test_makefile.Test_Makefile_name)
    gen_test_makefile.gen_makefile(test_makefile_location, True, config_file)
    common.check_and_copy(test_makefile_location, dest)
    return test_makefile_location


def dump_testing_sets(targets):
    test_sets = []
    for i in gen_test_makefile.CompilerTarget.all_targets:
        if i.specs.name in targets.split():
            test_sets.append(i.name)
    common.log_msg(logging.INFO, "Running "+str(len(test_sets))+" test sets: "+ str(test_sets), forced_duplication=True)


def print_compilers_version(targets):
    for i in targets.split():
        comp_exec_name = gen_test_makefile.CompilerSpecs.all_comp_specs[i].comp_name
        if not common.if_exec_exist(comp_exec_name):
            common.print_and_exit("Can't find " + comp_exec_name + " binary")
        ret_code, output, err_output, time_expired, elapsed_time = common.run_cmd([comp_exec_name, "--version"])
        # TODO: I hope it will work for all compilers
        common.log_msg(logging.DEBUG, str(output.splitlines()[0], "utf-8"))
        gen_test_makefile.CompilerSpecs.all_comp_specs[i].set_version(str(output.splitlines()[0], "utf-8"))


def process_seed_line(line):
    seeds = []
    line = line.replace(",", " ")
    l_seeds = line.split()
    for s1 in l_seeds:
        s2 = s1.lstrip("S_").rstrip("/")
        if not s2.isnumeric():
            common.print_and_exit("Seed "+s1+" can't be parsed")
        seeds.append(s2)
    return seeds


# Parse input of "seeds" options. Return the list of seeds.
def proccess_seeds(seeds_option_value):
    seeds = seeds_option_value.split()
    if len(seeds) == 1 and os.path.isfile(seeds[0]):
        seeds_file = common.check_and_open_file(seeds[0], "r")
        seeds = []
        for line in seeds_file:
            seeds += process_seed_line(line)
    else:
        seeds = process_seed_line(seeds_option_value)
    unique_seeds = list(set(seeds))
    common.log_msg(logging.INFO, "Running generator for "+str(len(unique_seeds))+" seeds. Seed are: ", forced_duplication=True);
    common.log_msg(logging.INFO, unique_seeds, forced_duplication=True)
    if len(unique_seeds) != len(seeds):
        common.log_msg(logging.INFO, "Note, that in the input seeds list there were "+str(len(seeds)-len(unique_seeds))+" duplicating seeds.", forced_duplication=True)
    return unique_seeds

def prepare_env_and_start_testing(out_dir, timeout, targets, num_jobs, config_file, seeds_option_value):
    common.check_dir_and_create(out_dir)

    # Check for binary of generator
    yarpgen_bin = os.path.abspath(common.yarpgen_home + os.sep + "yarpgen")
    common.check_and_copy(yarpgen_bin, out_dir)
    ret_code, output, err_output, time_expired, elapsed_time = common.run_cmd([yarpgen_bin, "-v"], yarpgen_timeout, 0)
    common.yarpgen_version_str = str(output, "utf-8")
    # TODO: need to add some check, but I hope that it is safe
    common.log_msg(logging.DEBUG, "YARPGEN version: " + common.yarpgen_version_str)

    test_makefile_location = gen_test_makefile_and_copy(out_dir, config_file)

    dump_testing_sets(targets)

    seeds = []
    task_queue = None
    if seeds_option_value:
        seeds = proccess_seeds(seeds_option_value)
        task_queue = multiprocessing.Queue()
        for s in seeds:
            task_queue.put(s)

    print_compilers_version(targets)

    os.chdir(out_dir)
    common.check_dir_and_create(res_dir)
    for i in range(num_jobs):
        common.check_dir_and_create(process_dir + str(i))
        common.check_and_copy(test_makefile_location, process_dir + str(i))

    lock = multiprocessing.Lock()
    manager_obj = manager()
    stat = manager_obj.Statistics()

    start_time = time.time()
    end_time = start_time + timeout * 60
    if timeout == -1:
        end_time = -1

    task_threads = [0] * num_jobs
    for num in range(num_jobs):
        task_threads[num] = multiprocessing.Process(target=gen_and_test, args=(num, lock, end_time, task_queue, stat, targets))
        task_threads[num].start()

    print_online_statistics(lock, stat, targets, task_threads, num_jobs)

    sys.stdout.write("\n")
    for i in range(num_jobs):
        common.log_msg(logging.DEBUG, "Removing " + process_dir + str(i) + " dir")
        shutil.rmtree(process_dir + str(i))

    stat_str, verbose_stat_str, prev_len = form_statistics(stat, targets, 0)
    sys.stdout.write(verbose_stat_str)
    sys.stdout.flush()


def gen_and_test(num, lock, end_time, task_queue, stat, targets):
    common.log_msg(logging.DEBUG, "Job #" + str(num))
    os.chdir(process_dir + str(num))
    inf = (end_time == -1) or not (task_queue is None)

    while inf or end_time > time.time():
        seed = ""
        if not task_queue is None:
            try:
                seed = task_queue.get_nowait()
            except queue.Empty:
                return

        # Generate the test.
        # TODO: maybe, it is better to call generator through Makefile?
        test = Test(stat=stat, seed=seed, proc_num=num)
        if not test.is_ok():
            test.save(lock)
            continue

        # Run all required opt-sets.
        out_res = set()
        prev_out_res_len = 1  # We can't check first result
        for t in gen_test_makefile.CompilerTarget.all_targets:
            # Skip the target we are not supposed to run.
            if t.specs.name not in targets.split():
                continue
            target_elapsed_time = 0.0
#            common.log_msg(logging.DEBUG, "From process #" + str(num) + ": " + str(output, "utf-8"))

            test_run = TestRun(stat=stat, target=t, seed=test.seed, proc_num=num)
            if not test_run.build():
                test_run.save(lock)
                continue

            if not test_run.run():
                test_run.save(lock)
                continue

            test.add_success_run(test_run)

        # Done with running tests, now verify the results.
        test.verify_results(lock)


def save_test(lock, num, seed, output, err_output, target, fail_tag):
    dest = ".." + os.sep + res_dir
    # Check and/or create compilers codename dir
    if target is not None:
        dest += os.sep + target.specs.name
    else:
        dest += os.sep + "gen_fail"
    lock.acquire()
    common.check_dir_and_create(dest)
    # Check and/or create fail_tag dir
    dest += os.sep + str(fail_tag)
    common.check_dir_and_create(dest)
    if target is not None and target.arch.sde_arch.name != "":
        dest += os.sep + target.arch.sde_arch.name
        common.check_dir_and_create(dest)
    dest += os.sep + "S_" + seed
    if os.path.exists(dest):
        if not os.path.isdir(dest):
            common.print_and_exit("Can't use '" + dest + "' directory")
            lock.release()
            return
    else:
        os.makedirs(dest)
    log = open(dest + os.sep + "log.txt", "a")
    common.log_msg(logging.DEBUG, "Saving test in " + str(num) + "thread to " + dest)

    log.write("YARPGEN version: " + common.yarpgen_version_str + "\n")
    log.write("Seed: " + str(seed) + "\n")
    log.write("Time: " + datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S') + "\n")
    log.write("Type: " + str(fail_tag) + "\n")
    # If it is generator's error, we can't copy test's source files
    if target is None:
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
    test_files.append(gen_test_makefile.Test_Makefile_name)
    for f in test_files:
        common.check_and_copy(f, dest)
    lock.release()


###############################################################################


class CustomFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawDescriptionHelpFormatter):
    pass

if __name__ == '__main__':
    if os.environ.get("YARPGEN_HOME") is None:
        sys.stderr.write("\nWarning: please set YARPGEN_HOME envirnoment variable to point to test generator path,"
                         " using " + common.yarpgen_home + " for now\n")

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
    parser = argparse.ArgumentParser(description=description, epilog=epilog, formatter_class=CustomFormatter)
    parser.add_argument("-o", "--output", dest="out_dir", default="testing", type=str,
                        help="Directory, which is used for testing.")
    parser.add_argument("-t", "--timeout", dest="timeout", type=int, default=1,
                        help="Timeout for test system in minutes. -1 means infinity")
    parser.add_argument("--target", dest="target", default="clang ubsan gcc", type=str,
                        help="Targets for testing (see test_sets.txt). By default, possible variants are "
                             "clang, ubsan and gcc (ubsan is a clang with sanitizer options).")
    parser.add_argument("-j", dest="num_jobs", default=multiprocessing.cpu_count(), type=int,
                        help='Maximum number of instances to run in parallel. By defaulti, it is set to'
                             ' number of processor in your system')
    parser.add_argument("--config-file", dest="config_file",
                        default=os.path.join(common.yarpgen_home, gen_test_makefile.default_test_sets_file_name),
                        type=str, help="Configuration file for testing")
    parser.add_argument("--log-file", dest="log_file", type=str,
                        help="Logfile")
    parser.add_argument("-v", "--verbose", dest="verbose", default=False, action="store_true",
                        help="Increase output verbosity")
    parser.add_argument("--stat-log-file", dest="stat_log_file", default="statistics.log", type=str,
                        help="Logfile")
    parser.add_argument("--seeds", dest="seeds_option_value", default="", type=str,
                        help="List of generator seeds to run or a file name with the list of seeds. "\
                             "Seeds may be separated by whitespaces and commas."\
                             "The seed may start with S_ or end with /, i.e. S_12345/ is interpretted as 12345.")
    args = parser.parse_args()

    log_level = logging.DEBUG if args.verbose else logging.INFO
    common.setup_logger(args.log_file, log_level)

    stat_log_file = common.wrap_log_file(args.stat_log_file, parser.get_default("stat_log_file"))
    common.setup_stat_logger(stat_log_file)

    script_start_time = datetime.datetime.now()
    common.log_msg(logging.DEBUG, "Start time: " + script_start_time.strftime('%Y/%m/%d %H:%M:%S'))
    common.check_python_version()
    prepare_env_and_start_testing(os.path.abspath(args.out_dir), args.timeout, args.target, args.num_jobs,
                                  args.config_file, args.seeds_option_value)
