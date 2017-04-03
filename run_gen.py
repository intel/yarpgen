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
import re
import shutil
import stat
import sys
import time
import queue

import common
import gen_test_makefile
import blame_opt

res_dir = "result"
process_dir = "process_"
creduce_bin = "creduce"

yarpgen_timeout = 60
compiler_timeout = 1200
run_timeout = 300
stat_update_delay = 10
tmp_cleanup_delay = 3600
creduce_timeout = 3600 * 24

script_start_time = datetime.datetime.now()  # We should init variable, so let's do it this way

known_build_fails = { \
# clang
    "SelectionDAGISel": "Assertion `NodeToMatch\-\>getOpcode\(\) != ISD::DELETED_NODE && \"NodeToMatch was removed partway through selection\"'", \
    "replaceAllUses" : "replaceAllUses of value with new value of different type", \
    "FoldCONCAT_VECTORS" : "Concatenation of vectors with inconsistent value types", \
    "PromoteIntRes_SETCC" : "Integer type overpromoted", \
    "Cannot_select_urem" : "Cannot select.*urem", \
    "Cannot_select_pcmpeq" : "Cannot select.*X86ISD::PCMPEQ", \
    "Binary_operator_types_must_match": "Binary operator types must match", \
    "DAGCombiner_AddToWorklist": "Deleted Node added to Worklist", \
    "SDNode_getOperand": "Invalid child # of SDNode", \
    "DELETED_NODE_CSEMap": "DELETED_NODE in CSEMap!", \
    "VerifyScheduledSequence": "The number of nodes scheduled doesn't match the expected number", \
# gcc
    "compute_live_loop_exits" : "compute_live_loop_exits", \
    "verify_gimple" : "verify_gimple" \
}

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
    STATUS_miscompare=4
    STATUS_multiple_miscompare=5

    # Generate new test
    # stat is statistics object
    # seed is optional, if we want to generate some particular seed.
    # proc_num is optinal debug info to track in what process we are running this activity.
    def __init__(self, stat, seed="", proc_num=-1, blame=False, creduce_makefile=None):
        # Run generator
        yarpgen_run_list = [".." + os.sep + "yarpgen", "-q"]
        if seed:
            yarpgen_run_list += ["-s", seed]
        self.yarpgen_cmd = " ".join(str(p) for p in yarpgen_run_list)
        self.ret_code, self.stdout, self.stderr, self.is_time_expired, self.elapsed_time = \
            common.run_cmd(yarpgen_run_list, yarpgen_timeout, proc_num)

        # Files that belongs to generate test. They are hardcoded for now.
        # Generator may report them in output later and we may need to parse it.
        self.files = gen_test_makefile.sources.value.split() + gen_test_makefile.headers.value.split()
        self.files.append(gen_test_makefile.Test_Makefile_name)

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
        self.blame = blame
        self.blame_phase = ""
        self.blame_result = "was not run"
        self.creduce = bool(creduce_makefile)
        self.creduce_makefile = creduce_makefile

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
        self.fail_test_runs = []

        seed_file = open("seed", "w")
        seed_file.write(self.seed + "\n")
        seed_file.close()
        common.log_msg(logging.DEBUG, "Process " + str(proc_num) + " has generated seed " + str(seed))

    # Check status
    def is_ok(self):
        return self.status == self.STATUS_ok

    def status_string(self):
        if self.status == self.STATUS_ok:                    return "ok"
        elif self.status == self.STATUS_fail:                return "gen_fail"
        elif self.status == self.STATUS_fail_timeout:        return "gen_fail_timeout"
        elif self.status == self.STATUS_miscompare:          return "miscompare"
        elif self.status == self.STATUS_multiple_miscompare: return "multiple_miscompare"
        else: raise

    # Save test
    def save(self, lock):
        if self.status != self.STATUS_fail_timeout and self.status != self.STATUS_fail:
            raise
        log = self.build_log()
        save_test(lock, file_list=[log],
                   compiler_name=None,
                   fail_type=self.status_string(),
                   classification=None,
                   test_name=None)

    # Add successful test run
    def add_success_run(self, test_run):
        self.successful_test_runs.append(test_run)

    # Add fail test run
    def add_fail_run(self, test_run):
        self.fail_test_runs.append(test_run)

    # Save failed runs and verify successul runs
    def handle_results(self, lock):
        # Handle compfails and runfails.
        self.save_failed(lock)
        # Handle miscompares.
        self.verify_results(lock)
        if self.status == self.STATUS_ok and len(self.fail_test_runs) == 0:
            self.stat.seed_passed(self.seed)
        else:
            self.stat.seed_failed(self.seed)

    # Save failed runs.
    # Report fails of the same type together.
    def save_failed(self, lock):
        build_fail = None
        run_fail = None
        for run in self.fail_test_runs:
            if run.status == TestRun.STATUS_compfail or \
               run.status == TestRun.STATUS_compfail_timeout:
                if build_fail:
                    build_fail.same_type_fails.append(run)
                else:
                    build_fail = run
            elif run.status == TestRun.STATUS_runfail or \
                 run.status == TestRun.STATUS_runfail_timeout:
                if run_fail:
                    run_fail.same_type_fails.append(run)
                else:
                    run_fail = run
            else:
                raise

        if build_fail:
            if self.creduce:
                self.do_creduce_buildfail(build_fail)
            build_fail.save(lock)
        if run_fail:
            # Do blaming if blame switch is passed, there are successful runs and fail is not a timeout.
            if self.blame and len(self.successful_test_runs) > 0 and run_fail.status == TestRun.STATUS_runfail:
                do_blame(run_fail, self.files, self.successful_test_runs[0].checksum, run_fail.target)
            run_fail.save(lock)

    # Verify the results and if bad results are found, report / save them.
    def verify_results(self, lock):
        results = {}
        for t in self.successful_test_runs:
            assert t.status == TestRun.STATUS_ok
            if t.checksum not in results:
                results[t.checksum] = [t]
            else:
                results[t.checksum].append(t)

        # Check if test passed.
        if len(results) == 1:
            return
        elif len(results) == 2:
            self.status = self.STATUS_miscompare
            runs = list(results.values())
            good_runs = runs[0]
            bad_runs = runs[1]
            # Majority vote
            if len(bad_runs) > len(good_runs):
                good_runs, bad_runs = bad_runs, good_runs
            # Count number of no_opt optsets in each bin.
            good_no_opt = 0
            for run in good_runs:
                good_no_opt += run.optset.count("no_opt")
            bad_no_opt = 0
            for run in bad_runs:
                bad_no_opt += run.optset.count("no_opt")
            if good_no_opt == 0 and bad_no_opt > 0:
                good_runs, bad_runs = bad_runs, good_runs
            # Assume one compiler is failing at many opt-sets.
            good_cmplrs = set()
            bad_cmplrs = set()
            for run in good_runs:
                good_cmplrs.add(run.target.specs.name)
            for run in bad_runs:
                bad_cmplrs.add(run.target.specs.name)
            if len(good_cmplrs) < len(bad_cmplrs):
                good_runs, bad_runs = bad_runs, good_runs
        else:
            # More than 2 different results.
            # Treat them all as bad
            self.status = self.STATUS_multiple_miscompare
            good_runs = []
            bad_runs = []
            for run in results.values():
                bad_runs += run

        # Run blame triagging for one of failing optsets
        if self.blame and good_runs:
            do_blame(self, self.files, good_runs[0].checksum, bad_runs[0].target)

        # Run creduce for one of failing optsets
        if self.creduce and good_runs:
            self.do_creduce_miscompare(good_runs, bad_runs)

        # Report
        for run in bad_runs:
            self.stat.update_target_runs(run.optset, out_dif)
        # Build log
        log = self.build_log(bad_runs, good_runs)
        self.files.append(log)

        # Save
        files_to_save = self.files
        for run in (bad_runs + good_runs):
            files_to_save.append(run.exe_file)
        cmplr_set = set()
        for run in bad_runs:
            cmplr_set.add(run.target.specs.name)
        cmplr_set = list(cmplr_set)
        cmplr_set.sort()
        cmplr = "-".join(c for c in cmplr_set)
        blame_phase = None
        if len(self.blame_phase) != 0:
            blame_phase = self.blame_phase.replace(" ", "_")

        save_test(lock, files_to_save,
                   compiler_name = cmplr,
                   fail_type = self.status_string(),
                   classification = blame_phase,
                   test_name = "S_" + str(self.seed))

    def build_log(self, bad_runs=[], good_runs=[]):
        log_name = "log.txt"
        log = open(log_name, "w")
        log.write("YARPGEN version: " + common.yarpgen_version_str + "\n")
        log.write("Seed: " + str(self.seed) + "\n")
        log.write("Time: " + datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S') + "\n")
        log.write("Type: " + self.status_string() + "\n")
        if self.blame:
            log.write("Blaming " + self.blame_result + "\n")
            log.write("Optimization to blame: " + self.blame_phase + "\n")
        log.write("\n\n")
        if self.status == self.STATUS_fail_timeout:
            log.write("Generator timeout: " + str(yarpgen_timeout) + " seconds\n")
        if self.status == self.STATUS_fail or self.status == self.STATUS_fail_timeout:
            log.write("Generator cmd: " + self.yarpgen_cmd + "\n")
            log.write("Generator exit code: " + str(self.ret_code) + "\n")
            log.write("=== Generator log ==================================================\n")
            log.write(str(self.stdout, "utf-8"))
            log.write("=== Generator err ==================================================\n")
            log.write(str(self.stderr, "utf-8"))
            log.write("=== Generator end ==================================================\n")
            log.write("\n")
        if self.status >= self.STATUS_miscompare:
            for run in bad_runs:
                log.write("==== BAD ==================================\n")
                log.write("Optset: " + run.optset + "\n")
                log.write("Output: " + str(run.run_stdout, "utf-8") + "\n")
            log.write("===========================================\n\n")
            for run in good_runs:
                log.write("==== GOOD =================================\n")
                log.write("Optset: " + run.optset + "\n")
                log.write("Output: " + str(run.run_stdout, "utf-8") + "\n")
            log.write("===========================================\n")

        log.close()
        return log_name

    def creduce_performance_hack(self):
        # 1. Move iostream include to driver.cpp
        # 2. Remove array, vector and valarray if the are not used
        init_h = open("init.h")
        init_h_content = init_h.read()
        init_h.close()
        func_cpp = open("func.cpp")
        func_cpp_content = func_cpp.read()
        func_cpp.close()
        iostream = re.compile("iostream")
        array    = re.compile("std::array")
        vector   = re.compile("std::vector")
        valarray = re.compile("std::valarray")

        remove_iostream = len(iostream.findall(init_h_content)) != 0
        remove_array = len(array.findall(init_h_content)) == 0 and \
                       len(array.findall(func_cpp_content)) == 0
        remove_vector = len(vector.findall(init_h_content)) == 0 and \
                        len(vector.findall(func_cpp_content)) == 0
        remove_valarray = len(valarray.findall(init_h_content)) == 0 and \
                          len(valarray.findall(func_cpp_content)) == 0

        if remove_iostream:
            driver_cpp = open("driver.cpp")
            driver_cpp_content = driver_cpp.read()
            driver_cpp.close()
            driver_cpp = open("driver.cpp", "w")
            driver_cpp.write("#include <iostream>\n")
            driver_cpp.write(driver_cpp_content)
            driver_cpp.close()

        if remove_iostream or remove_array or remove_vector or remove_valarray:
            init_h = open("init.h")
            init_h_content = init_h.readlines()
            init_h.close()
            init_h = open("init.h", "w")
            for l in init_h_content:
                if not (remove_iostream and re.search("\<iostream\>", l) or \
                        remove_array and re.search("\<array\>", l) or \
                        remove_vector and re.search("\<vector\>", l) or \
                        remove_valarray and re.search("\<valarray\>", l)):
                    init_h.write(l)
            init_h.close()

    def do_creduce_miscompare(self, good_runs, bad_runs):
        # Pick the fastest non-failing opt-set
        good_run = None
        good_time = 0
        for run in good_runs:
            if not run.target.specs.name.startswith("ubsan"):
                continue
            time = run.build_elapsed_time + run.run_elapsed_time
            if good_run is None or time < good_time:
                good_run, good_time = run, time

        if not good_run:
            # TODO: fix
            common.log_msg(logging.DEBUG, "No good runs found for seed " + self.seed + \
                    " in process " + self.proc_num)
            raise

        bad_run = None
        bad_time = 0
        for run in bad_runs:
            time = run.build_elapsed_time + run.run_elapsed_time
            if bad_run is None or time < bad_time:
                bad_run, bad_time = run, time
        common.log_msg(logging.DEBUG, "Running creduce for seed " + self.seed + ": " + \
                good_run.optset + " vs " + bad_run.optset)

        # Prepare Makefile
        common.check_and_copy(self.creduce_makefile, ".")
        creduce_makefile_name = os.path.basename(self.creduce_makefile)
        self.files.append(creduce_makefile_name)

        # Prepare reduce folder
        os.mkdir("reduce")
        for f in self.files:
            common.check_and_copy(f, "reduce")
        os.chdir("reduce")

        self.creduce_performance_hack()

        # Now we need to construct a Makefile and script for passing to creduce.
        # Need to make sure that -Werror=uninitialized is passed to the compiler.
        # Recommended flags:
        #   -O0 -fsanitize=undefined -fno-sanitize-recover=undefined -w -Werror=uninitialized
        test_sh = "#!/bin/bash\n\n"
        test_sh +="ulimit -t " + str(max(compiler_timeout, run_timeout)) + "\n\n"
        test_sh +="export TEST_PWD="+os.getcwd()+"\n\n"
        test_sh +="make -f $TEST_PWD" + os.sep + creduce_makefile_name + " " + good_run.optset + " &&\\\n"
        test_sh +="make -f $TEST_PWD" + os.sep + creduce_makefile_name + " run_" + good_run.optset + " > no_opt_out &&\\\n"
        test_sh +="make -f $TEST_PWD" + os.sep + creduce_makefile_name + " " + bad_run.optset + " &&\\\n"
        test_sh +="make -f $TEST_PWD" + os.sep + creduce_makefile_name + " run_" + bad_run.optset + " > opt_out &&\\\n"
        test_sh +="! diff no_opt_out opt_out"
        test_sh_file = open("test.sh", "w")
        test_sh_file.write(test_sh)
        test_sh_file.close()
        st = os.stat(test_sh_file.name)
        os.chmod(test_sh_file.name, st.st_mode | stat.S_IEXEC)
        common.check_and_copy(test_sh_file.name, "..")
        self.files.append(test_sh_file.name)

        # Run creduce
        cr_params_list = [creduce_bin, "--timing", "--timeout", str((run_timeout+compiler_timeout)*2), os.path.abspath(test_sh_file.name), "func.cpp"]
        cr_cmd = " ".join(str(p) for p in cr_params_list)
        cr_ret_code, cr_stdout, cr_stderr, cr_is_time_expired, cr_elapsed_time = \
            common.run_cmd(cr_params_list, creduce_timeout, self.proc_num)

        # Store results and copy them back
        cr_out = open("creduce_" + bad_run.optset + ".out", "w")
        cr_out.write(str(cr_stdout, "utf-8"))
        cr_out.close()
        common.check_and_copy(cr_out.name, "..")
        self.files.append(cr_out.name)

        cr_err = open("creduce_" + bad_run.optset + ".err", "w")
        cr_err.write(str(cr_stderr, "utf-8"))
        cr_err.close()
        common.check_and_copy(cr_err.name, "..")
        self.files.append(cr_err.name)

        cr_log = open("creduce_" + bad_run.optset + ".log", "w")
        cr_log.write("Return code: " + str(cr_ret_code) + "\n")
        cr_log.write("Execution time: " + str(cr_elapsed_time) + "\n")
        cr_log.write("Time limit was exceeded!\n" if cr_is_time_expired else "Time limit was not exceeded\n")
        cr_log.close()
        common.check_and_copy(cr_log.name, "..")
        self.files.append(cr_log.name)

        reduced_file_name = "func_reduced_" + bad_run.optset + ".cpp"
        common.check_and_copy("func.cpp", ".." + os.sep+reduced_file_name)
        self.files.append(reduced_file_name)

        os.chdir(self.path)

    def do_creduce_buildfail(self, buildfail_run):
        # Pick a ubsan run.
        ubsan_run = None
        ubsan_time = 0
        for run in self.successful_test_runs:
            if not run.target.specs.name.startswith("ubsan"):
                continue
            time = run.build_elapsed_time + run.run_elapsed_time
            if ubsan_run is None or time < ubsan_time:
                ubsan_run, ubsan_time = run, time

        common.log_msg(logging.DEBUG, "Running creduce (compfail) for seed " + self.seed + ": " + \
                buildfail_run.optset + " with " + (ubsan_run.optset if ubsan_run else "missing ubsan"))

        if not ubsan_run:
            common.log_msg(logging.DEBUG, "Failed to reduce because of missing good ubsan run: seed " + \
                self.seed)
            return

        # Prepare Makefile
        common.check_and_copy(self.creduce_makefile, ".")
        creduce_makefile_name = os.path.basename(self.creduce_makefile)
        self.files.append(creduce_makefile_name)

        # Prepare reduce folder
        os.mkdir("reduce_compfail")
        for f in self.files:
            common.check_and_copy(f, "reduce_compfail")
        os.chdir("reduce_compfail")

        # Now we need to construct a Makefile and script for passing to creduce.
        # Need to make sure that -Werror=uninitialized is passed to the compiler.
        # Recommended flags:
        #   -O0 -fsanitize=undefined -fno-sanitize-recover=undefined -w -Werror=uninitialized
        test_sh = "#!/bin/bash\n\n"
        test_sh +="ulimit -t " + str(compiler_timeout) + "\n\n"
        test_sh +="export TEST_PWD="+os.getcwd()+"\n\n"
        test_sh +="! make -f $TEST_PWD" + os.sep + creduce_makefile_name + " " + buildfail_run.optset + " &&\\\n"
        test_sh +="make -f $TEST_PWD" + os.sep + creduce_makefile_name + " " + ubsan_run.optset + " &&\\\n"
        test_sh +="make -f $TEST_PWD" + os.sep + creduce_makefile_name + " run_" + ubsan_run.optset + " \n"
        test_sh_file = open("test.sh", "w")
        test_sh_file.write(test_sh)
        test_sh_file.close()
        st = os.stat(test_sh_file.name)
        os.chmod(test_sh_file.name, st.st_mode | stat.S_IEXEC)
        common.check_and_copy(test_sh_file.name, "..")
        self.files.append(test_sh_file.name)

        # Run creduce
        cr_params_list = [creduce_bin, "--timing", "--timeout", str((run_timeout+compiler_timeout)*2), os.path.abspath(test_sh_file.name), "func.cpp"]
        cr_cmd = " ".join(str(p) for p in cr_params_list)
        cr_ret_code, cr_stdout, cr_stderr, cr_is_time_expired, cr_elapsed_time = \
            common.run_cmd(cr_params_list, creduce_timeout, self.proc_num)

        # Store results and copy them back
        cr_out = open("creduce_" + buildfail_run.optset + ".out", "w")
        cr_out.write(str(cr_stdout, "utf-8"))
        cr_out.close()
        common.check_and_copy(cr_out.name, "..")
        self.files.append(cr_out.name)

        cr_err = open("creduce_" + buildfail_run.optset + ".err", "w")
        cr_err.write(str(cr_stderr, "utf-8"))
        cr_err.close()
        common.check_and_copy(cr_err.name, "..")
        self.files.append(cr_err.name)

        cr_log = open("creduce_" + buildfail_run.optset + ".log", "w")
        cr_log.write("Return code: " + str(cr_ret_code) + "\n")
        cr_log.write("Execution time: " + str(cr_elapsed_time) + "\n")
        cr_log.write("Time limit was exceeded!\n" if cr_is_time_expired else "Time limit was not exceeded\n")
        cr_log.close()
        common.check_and_copy(cr_log.name, "..")
        self.files.append(cr_log.name)

        reduced_file_name = "func_reduced_" + buildfail_run.optset+".cpp"
        common.check_and_copy("func.cpp", ".." + os.sep + reduced_file_name)
        self.files.append(reduced_file_name)

        os.chdir(self.path)


# End of Test class

# TODO:
# 1. save test
# 2. generate log
# 3. diagnostic logic
# 4. rerun test from the dir?
# End of Test class

# class representing the result of running a single opt-set for the test
class TestRun(object):
    # opt-set
    # build / execution log
    # build / execution time
    # run result: pass / compfail / compfail_timeout / runfail / runfail_timeout / miscompare.
    # compilation log

    STATUS_not_built = 1
    STATUS_not_run = 2
    STATUS_compfail = 3
    STATUS_compfail_timeout = 4
    STATUS_runfail = 5
    STATUS_runfail_timeout = 6
    STATUS_ok = 7
    STATUS_miscompare = 8

    def __init__(self, test, stat, target, proc_num=-1):
        self.test = test
        self.stat = stat
        self.target = target
        self.optset = target.name
        self.proc_num = proc_num
        self.status = self.STATUS_not_built
        self.files = []
        self.same_type_fails = []
        self.blame_phase = ""
        self.blame_result = "was not run"

    # Build test
    def build(self):
        # build
        build_params_list = ["make", "-f", gen_test_makefile.Test_Makefile_name, self.optset]
        self.build_cmd = " ".join(str(p) for p in build_params_list)
        self.build_ret_code, self.build_stdout, self.build_stderr, self.is_build_time_expired, self.build_elapsed_time = \
            common.run_cmd(build_params_list, compiler_timeout, self.proc_num)
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
        expected_files = ["init.o", "driver.o", "func.o", "check.o", "hash.o", "out"]
        expected_files = [ self.optset + "_" + e for e in expected_files ]
        for f in expected_files:
            if os.path.isfile(f):
                self.files.append(f)
        # save executable separately (we need it in case of miscompare)
        exe_file = self.optset + "_out"
        if os.path.isfile(exe_file):
            self.exe_file = exe_file
        return self.status == self.STATUS_not_run

    # Run test
    def run(self):
        # run
        run_params_list = ["make", "-f", gen_test_makefile.Test_Makefile_name, "run_" + self.optset]
        self.run_cmd = " ".join(str(p) for p in run_params_list)
        self.run_ret_code, self.run_stdout, self.run_stderr, self.run_is_time_expired, self.run_elapsed_time = \
            common.run_cmd(run_params_list, run_timeout, self.proc_num)
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

    def status_string(self):
        if   self.status == self.STATUS_ok:               return "ok"
        elif self.status == self.STATUS_miscompare:       return "miscompare"
        elif self.status == self.STATUS_compfail:         return "compfail"
        elif self.status == self.STATUS_compfail_timeout: return "compfail_timeout"
        elif self.status == self.STATUS_runfail:          return "runfail"
        elif self.status == self.STATUS_runfail_timeout:  return "runfail_timeout"
        elif self.status == self.STATUS_not_built:        return "not_built"
        elif self.status == self.STATUS_not_run:          return "not_run"
        else: raise

    def save(self, lock):
        if self.status <= self.STATUS_not_built or self.status >= self.STATUS_miscompare:
            raise

        save_status = self.status_string()

        # TODO: it's the place to add a hook for build and run classification:
        classification = None
        if self.status == self.STATUS_compfail:
            # classify compfail
            res = self.classify_build_fail()
            if res is not None:
                classification = res
        elif self.status == self.STATUS_runfail:
            # classify runfail
            # use blame info
            if len(self.blame_phase) != 0:
                classification = self.blame_phase.replace(" ", "_")

        # Files to save: source files, own files, files from similar fails and
        # log file.
        file_list = self.test.files + self.files
        for run in self.same_type_fails:
            file_list += run.files
        # Remove duplicates
        file_list = list(set(file_list))
        log = self.build_log()
        file_list.append(log)

        save_test(lock,
                   file_list,
                   compiler_name=self.target.specs.name,
                   fail_type=save_status,
                   classification=classification,
                   test_name="S_"+str(self.test.seed))

    def classify_build_fail(self):
        for tag, reg_expr in known_build_fails.items():
            if re.search(reg_expr, str(self.build_stderr, "utf-8")):
                return tag
        return None

    def build_log(self):
        log_name = "log.txt"
        log = open(log_name, "w")
        log.write("YARPGEN version: " + common.yarpgen_version_str + "\n")
        log.write("Seed: " + str(self.test.seed) + "\n")
        log.write("Time: " + datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S') + "\n")
        tests = [self] + self.same_type_fails
        log.write("Optsets: " + ", ".join(t.optset for t in tests) + "\n")
        log.write("Type: " + self.status_string() + "\n")
        if self.test.blame:
            log.write("Blaming " + self.blame_result + "\n")
            log.write("Optimization to blame: " + self.blame_phase + "\n")

        for test in tests:
            log.write("\n\n")
            log.write("====================================================================\n")
            log.write("========== Details for " + test.optset + " optset.\n")
            log.write("====================================================================\n")
            if test.status == self.STATUS_compfail_timeout:
                log.write("Build timeout: " + str(compiler_timeout) + " seconds\n")
            if test.status >= self.STATUS_compfail:
                log.write("Build cmd: " + test.build_cmd + "\n")
                log.write("Build exit code: " + str(test.build_ret_code) + "\n")
                log.write("=== Build log ======================================================\n")
                log.write(str(test.build_stdout, "utf-8"))
                log.write("=== Build err ======================================================\n")
                log.write(str(test.build_stderr, "utf-8"))
                log.write("=== Build end ======================================================\n")
                log.write("\n")
            if test.status == self.STATUS_runfail_timeout:
                log.write("Build timeout: " + str(run_timeout) + " seconds\n")
            if test.status >= self.STATUS_runfail:
                log.write("Run cmd: " + test.run_cmd + "\n")
                log.write("Run exit code: " + str(test.run_ret_code) + "\n")
                log.write("=== Run log ========================================================\n")
                log.write(str(test.run_stdout, "utf-8"))
                log.write("=== Run err ========================================================\n")
                log.write(str(test.run_stderr, "utf-8"))
                log.write("=== Run end ========================================================\n")
                log.write("\n")

        log.close()
        return log_name
# End of TestRun class

# Run blaming in Test or TestRun object.
# out: new files, blame_phase, blame_result
def do_blame(test_obj, test_files, good_result, target_to_blame):
    current_dir = os.getcwd()
    try:
        # Prepare sandbox for blaming run
        os.mkdir("blame")
        for f in test_files:
            common.check_and_copy(f, "blame")
        # Run blaming
        blame_phase = blame_opt.prepare_env_and_blame(
            fail_dir = os.path.join(current_dir, "blame"),
            valid_res = good_result,
            fail_target = target_to_blame,
            out_dir = "./blame",
            lock = None,
            num = test_obj.proc_num,
            inplace = True)
        # Copy resuls back
        os.chdir(current_dir)
        if os.path.exists("blame/Blame_Makefile"):
            common.check_and_copy("blame/Blame_Makefile", ".")
            test_obj.files.append("Blame_Makefile")
        if os.path.exists("blame/log.txt"):
            common.check_and_copy("blame/log.txt", "./blame.log")
            test_obj.files.append("blame.log")
        # interpret results
        if type(blame_phase) is str:
            test_obj.blame_phase = blame_phase
            test_obj.blame_result = "was successul"
        else:
            test_obj.blame_result = "has failed"
    except Exception as e:
        test_obj.blame_result = "raised an exception"
        common.log_msg(logging.WARNING, "Exception was raised during blaming: "+ str(e))
    os.chdir(current_dir)


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
        self.seeds_pass = None
        self.seeds_fail = None

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

    def enable_seeds(self):
        self.seeds_pass = []
        self.seeds_fail = []

    def seeds_enabled(self):
        return not self.seeds_pass is None

    def get_seeds(self):
        return self.seeds_pass, self.seeds_fail

    def seed_passed(self, seed):
        if not self.seeds_pass is None:
            self.seeds_pass.append(seed)

    def seed_failed(self, seed):
        if not self.seeds_fail is None:
            self.seeds_fail.append(seed)

MyManager.register("Statistics", Statistics)


def strfdelta(time_delta, format_str):
    time_dict = {"days": time_delta.days}
    time_dict["hours"], rem = divmod(time_delta.seconds, 3600)
    time_dict["minutes"], time_dict["seconds"] = divmod(rem, 60)
    return format_str.format(**time_dict)


def get_testing_speed(seed_num, time_delta):
    minutes = time_delta.total_seconds() / 60
    return "{:.2f}".format(seed_num / minutes) + " seed/min"


def form_statistics(stat, targets, prev_len, tasks = None):
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

    if stat.seeds_enabled():
        seeds_pass, seeds_fail = stat.get_seeds()
        verbose_stat_str += "PASSED SEEDS (" + str(len(seeds_pass)) + "): " + \
                            ", ".join("S_"+s for s in seeds_pass) + "\n"
        verbose_stat_str += "FAILED SEEDS (" + str(len(seeds_fail)) + "): " + \
                            ", ".join("S_"+s for s in seeds_fail) + "\n"

    active = 0
    if tasks:
        for task in tasks:
            if task.is_alive():
                active = active + 1

    stat_str = '\r'
    stat_str += "time " + strfdelta(datetime.datetime.now() - script_start_time,
                                    "{days} d {hours}:{minutes}:{seconds}") + " | "
    stat_str += "cpu time: " + strfdelta(total_cpu_duration, "{days} d {hours}:{minutes}:{seconds}") + " | "
    stat_str += testing_speed + " | "
    stat_str += " active " + str(active) + " | "
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


# Print realtime stats and also run tmp cleaner in background.
def print_online_statistics_and_cleanup(lock, stat, targets, task_threads, num_jobs, no_tmp_cln):
    any_alive = True
    prev_len = 0
    start_time = time.time() - tmp_cleanup_delay
    while any_alive:
        lock.acquire()
        stat_str, verbose_stat_str, prev_len = form_statistics(stat, targets, prev_len, task_threads)
        common.stat_logger.log(logging.INFO, verbose_stat_str)
        sys.stdout.write(stat_str)
        sys.stdout.flush()
        lock.release()

        if (time.time() - start_time) > tmp_cleanup_delay and not no_tmp_cln:
            start_time = time.time()
            common.run_cmd([os.path.abspath(common.yarpgen_home + os.sep + "tmp_cleaner.sh")])

        time.sleep(stat_update_delay)

        any_alive = False
        for num in range(num_jobs):
            any_alive |= task_threads[num].is_alive()


def gen_test_makefile_and_copy(dest, config_file):
    test_makefile = os.path.abspath(os.path.join(dest, gen_test_makefile.Test_Makefile_name))
    gen_test_makefile.gen_makefile(test_makefile, True, config_file)
    return test_makefile


def dump_testing_sets(targets):
    test_sets = []
    for i in gen_test_makefile.CompilerTarget.all_targets:
        if i.specs.name in targets.split():
            test_sets.append(i.name)
    common.log_msg(logging.INFO, "Running "+str(len(test_sets))+" test sets: "+ str(test_sets), forced_duplication=True)


def print_compilers_version(targets):
    for i in targets.split():
        if i not in gen_test_makefile.CompilerSpecs.all_comp_specs:
            common.print_and_exit("Know nothing about " + i + " target")
        comp_exec_name = gen_test_makefile.CompilerSpecs.all_comp_specs[i].comp_name
        if not common.if_exec_exist(comp_exec_name):
            common.print_and_exit("Can't find " + comp_exec_name + " binary")
        ret_code, output, err_output, time_expired, elapsed_time = common.run_cmd([comp_exec_name, "--version"])
        # TODO: I hope it will work for all compilers
        common.log_msg(logging.DEBUG, str(output.splitlines()[0], "utf-8"))
        gen_test_makefile.CompilerSpecs.all_comp_specs[i].set_version(str(output.splitlines()[0], "utf-8"))


def check_creduce_version():
    try:
        ret_code, stdout, stderr, time_expired, elapsed_time = common.run_cmd([creduce_bin, "--help"], yarpgen_timeout, 0)
        stdout_str = str(stdout, "utf-8")
        match = re.match("creduce (\d)\.(\d)\.(\d).*-- a C and C\+\+ program reducer", stdout_str)
        if not match:
            common.print_and_exit("Can't read creduce version.")
        major = int(match.group(1))
        minor = int(match.group(2))
        patch = int(match.group(3))
        if major < 2 or (major == 2 and minor < 6):
            common.log_msg(logging.WARNING, "CReduce version 2.6.0 or later is recommended. "\
                    "You have version " + str(major) + "." + str(minor) + "." + str(patch))

    except FileNotFoundError as e:
        common.print_and_exit("CReduce is not found.")
    except Exception as e:
        print(str(e))
        common.print_and_exit("Problem with running CReduce.")


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
            if line.lstrip().startswith("#"):
                continue
            seeds += process_seed_line(line)
    else:
        seeds = process_seed_line(seeds_option_value)
    unique_seeds = list(set(seeds))
    unique_seeds.sort()
    common.log_msg(logging.INFO, "Running generator for "+str(len(unique_seeds))+" seeds. Seed are: ", forced_duplication=True);
    common.log_msg(logging.INFO, unique_seeds, forced_duplication=True)
    if len(unique_seeds) != len(seeds):
        common.log_msg(logging.INFO, "Note, that in the input seeds list there were "+str(len(seeds)-len(unique_seeds))+" duplicating seeds.", forced_duplication=True)
    return unique_seeds

def prepare_env_and_start_testing(out_dir, timeout, targets, num_jobs, config_file, seeds_option_value, blame, creduce, no_tmp_cln):
    common.check_dir_and_create(out_dir)

    # Check for binary of generator
    yarpgen_bin = os.path.abspath(common.yarpgen_home + os.sep + "yarpgen")
    common.check_and_copy(yarpgen_bin, out_dir)
    ret_code, output, err_output, time_expired, elapsed_time = common.run_cmd([yarpgen_bin, "-v"], yarpgen_timeout, 0)
    common.yarpgen_version_str = str(output, "utf-8")
    # TODO: need to add some check, but I hope that it is safe
    common.log_msg(logging.DEBUG, "YARPGEN version: " + common.yarpgen_version_str)

    # Check creduce version and generate Makefile for creduce
    creduce_makefile = None
    if creduce:
        check_creduce_version()
        creduce_makefile = os.path.abspath(os.path.join(out_dir, "CReduce_Makefile"))
        gen_test_makefile.gen_makefile(
                out_file_name = creduce_makefile,
                force = True,
                config_file = config_file,
                creduce_file = "func.cpp")

    makefile = os.path.abspath(os.path.join(out_dir, gen_test_makefile.Test_Makefile_name))
    gen_test_makefile.gen_makefile(
        out_file_name = makefile,
        force = True,
        config_file = config_file)

    dump_testing_sets(targets)

    seeds = []
    task_queue = None
    if seeds_option_value:
        seeds = proccess_seeds(seeds_option_value)
        task_queue = multiprocessing.Queue()
        for s in seeds:
            task_queue.put(s)
        if len(seeds) < num_jobs:
            num_jobs = len(seeds)

    print_compilers_version(targets)

    os.chdir(out_dir)
    common.check_dir_and_create(res_dir)
    for i in range(num_jobs):
        common.check_dir_and_create(process_dir + str(i))

    lock = multiprocessing.Lock()
    manager_obj = manager()
    stat = manager_obj.Statistics()
    if seeds_option_value:
        stat.enable_seeds()

    start_time = time.time()
    end_time = start_time + timeout * 60
    if timeout == -1:
        end_time = -1

    task_threads = [0] * num_jobs
    for num in range(num_jobs):
        task_threads[num] = multiprocessing.Process(target=gen_and_test,
            args=(num, makefile, lock, end_time, task_queue, stat, targets, blame, creduce_makefile))
        task_threads[num].start()

    print_online_statistics_and_cleanup(lock, stat, targets, task_threads, num_jobs, no_tmp_cln)

    sys.stdout.write("\n")
    for i in range(num_jobs):
        common.log_msg(logging.DEBUG, "Removing " + process_dir + str(i) + " dir")
        shutil.rmtree(process_dir + str(i))

    stat_str, verbose_stat_str, prev_len = form_statistics(stat, targets, 0)
    sys.stdout.write(verbose_stat_str)
    sys.stdout.flush()


def gen_and_test(num, makefile, lock, end_time, task_queue, stat, targets, blame, creduce_makefile):
    common.log_msg(logging.DEBUG, "Job #" + str(num))
    os.chdir(process_dir + str(num))
    work_dir = os.getcwd()
    inf = (end_time == -1) or not (task_queue is None)

    while inf or end_time > time.time():
        # Fetch next seed if seeds were specified
        seed = ""
        if task_queue is not None:
            # Python multiprocessing queue may raise empty exception
            # even for non empty queue, so do several attempts to not loos workers.
            for i in range(3):
                try:
                    seed = task_queue.get_nowait()
                except queue.Empty:
                    time.sleep(1/(num+1))
                    seed = "done"
                else:
                    break
            if seed == "done":
                break

        # Cleanup before start
        if os.getcwd() != work_dir:
            raise
        common.clean_dir(".")
        common.check_and_copy(makefile, work_dir)

        # Generate the test.
        # TODO: maybe, it is better to call generator through Makefile?
        test = Test(stat=stat, seed=seed, proc_num=num, blame=blame,
                    creduce_makefile=creduce_makefile)
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

            test_run = TestRun(test=test, stat=stat, target=t, proc_num=num)
            if not test_run.build():
                test.add_fail_run(test_run)
                continue

            if not test_run.run():
                test.add_fail_run(test_run)
                continue

            test.add_success_run(test_run)

        # Done with running tests, now verify the results.
        test.handle_results(lock)

    # Here we are done with this worker. Make a log entry and leave a marker in work dir.
    common.log_msg(logging.DEBUG, "Process " + str(num) + " is done working.")
    seed_file = open("done", "w")
    seed_file.write("Process " + str(num) + " is done working \n")
    seed_file.close()


# save file_list in [compiler_name]/[fail_type]/[classification]/[test_name]
# for example:
# - icc/miscompare/S_123456
# - icc/miscompare/SIMP/S_123456
# - clang/build_fail/assert_XXXX/S_123456
# - gcc/miscompare/S_123456
# - gen_fail/S_20161230_22_30
# return dir name
def save_test(lock, file_list, compiler_name=None, fail_type=None, classification=None, test_name=None):
    dest = ".." + os.sep + res_dir + \
                  ((os.sep + compiler_name) if (compiler_name is not None) else "") + \
                  ((os.sep + fail_type) if (fail_type is not None) else os.sep + "script_problem") + \
                  ((os.sep + classification) if (classification is not None) else "") + \
                  ((os.sep + test_name) if (test_name is not None) else os.sep + "FAIL_" + datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
    try:
        lock.acquire()
        dest = os.path.abspath(dest)
        common.check_dir_and_create(dest)
        for f in file_list:
            common.check_and_copy(f, dest)
    except Exception as e:
        common.log_msg(logging.ERROR, "Problem when saving test in " + str(dest) + " directory")
        common.log_msg(logging.ERROR, "Exception type: " + str(type(e)))
        common.log_msg(logging.ERROR, "Exception args: " + str(e.args))
        common.log_msg(logging.ERROR, "Exception: " + str(e))

    finally:
        lock.release()

    return dest


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
        run_gen.py -c "gcc clang ubsan_clang" -t -1
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
    parser.add_argument("--target", dest="target", default="clang ubsan_clang gcc", type=str,
                        help="Targets for testing (see test_sets.txt). By default, possible variants are "
                             "clang, ubsan_clang and gcc (ubsan_clang is a clang with sanitizer options).")
    parser.add_argument("-j", dest="num_jobs", default=multiprocessing.cpu_count(), type=int,
                        help='Maximum number of instances to run in parallel. By defaulti, it is set to'
                             ' number of processor in your system')
    parser.add_argument("--config-file", dest="config_file",
                        default=os.path.join(common.yarpgen_home, gen_test_makefile.default_test_sets_file_name),
                        type=str, help="Configuration file for testing")
    parser.add_argument("--log-file", dest="log_file", type=str,
                        help="Logfile. By default it's name of output dir + .log")
    parser.add_argument("-v", "--verbose", dest="verbose", default=False, action="store_true",
                        help="Increase output verbosity")
    parser.add_argument("--stat-log-file", dest="stat_log_file", default="statistics.log", type=str,
                        help="Logfile for statistics")
    parser.add_argument("--seeds", dest="seeds_option_value", default="", type=str,
                        help="List of generator seeds to run or a file name with the list of seeds. "\
                             "Seeds may be separated by whitespaces and commas."\
                             "The seed may start with S_ or end with /, i.e. S_12345/ is interpretted as 12345."
                             "File comments may start with #")
    parser.add_argument("--blame", dest="blame", default=False, action="store_true",
                        help="Enable optimization triagging for failing tests for supported compilers")
    parser.add_argument("--creduce", dest="creduce", default=False, action="store_true",
                        help="Enable test reduction using CReduce tool")
    parser.add_argument("--no-tmp-cleaner", dest="no_tmp_cleaner", default=False, action="store_true",
                        help="Do not run tmp_cleaner.sh script during the run")
    args = parser.parse_args()

    log_level = logging.DEBUG if args.verbose else logging.INFO
    if args.log_file is None:
        args.log_file = args.out_dir + ".log"
    common.setup_logger(args.log_file, log_level)

    stat_log_file = common.wrap_log_file(args.stat_log_file, parser.get_default("stat_log_file"))
    if len(args.seeds_option_value) != 0:
        stat_log_file = None
    common.setup_stat_logger(stat_log_file)

    script_start_time = datetime.datetime.now()
    common.log_msg(logging.DEBUG, "Command line: " + " ".join(str(p) for p in sys.argv))
    common.log_msg(logging.DEBUG, "Start time: " + script_start_time.strftime('%Y/%m/%d %H:%M:%S'))
    common.check_python_version()
    prepare_env_and_start_testing(os.path.abspath(args.out_dir), args.timeout, args.target, args.num_jobs,
                                  args.config_file, args.seeds_option_value, args.blame, args.creduce, args.no_tmp_cleaner)
