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
Experimental script for automatic sorting of errors, basing on failed optimization phase
"""
###############################################################################

import logging
import os
import re


import common
import gen_test_makefile
import run_gen


icc_blame_opts = ["-from_rtn=0 -to_rtn=", "-num_opt=", "-num-case="]
icc_opt_patterns = ["\(\d+\)", "\(\d+\)\s*\n", "DO ANOTHER.*\(\d+\)"]
icc_opt_name_prefix = "DOING\s*\[\w*\]\s*"
icc_opt_name_suffix = "\s*\(\d*\)\s*\(last opt\)"

icx_blame_opts = ["-mllvm -opt-bisect-limit="]
icx_opt_patterns = ["BISECT: running pass \(\d+\)"]
icx_opt_name_prefix = "BISECT: running pass \(\d+\) "
icx_opt_name_suffix = " \(.*\)"

clang_blame_opts = ["-mllvm -opt-bisect-limit="]
clang_opt_patterns = ["BISECT: running pass \(\d+\)"]
clang_opt_name_prefix = "BISECT: running pass \(\d+\) "
clang_opt_name_suffix = " \(.*\)"

dpcpp_gpu_blame_opts = ["IGC_ShaderDumpEnableAll=1 IGC_ShaderDisableOptPassesAfter="]
dpcpp_gpu_patterns = ["Skipping optimization pass: .* (threshold: \(\d+\))."]
dpcpp_gpu_opt_name_prefix = "Skipping optimization pass: '"
dpcpp_gpu_opt_name_suffix = "' \(.*\)."

compilers_blame_opts = {"icc": icc_blame_opts, "icx": icx_blame_opts, "clang": clang_blame_opts, "dpcpp": dpcpp_gpu_blame_opts}
compilers_blame_patterns = {"icc": icc_opt_patterns, "icx": icx_opt_patterns, "clang": clang_opt_patterns, "dpcpp": dpcpp_gpu_patterns}
compilers_opt_name_cutter = {"icc": [icc_opt_name_prefix, icc_opt_name_suffix], \
                             "icx": [icx_opt_name_prefix, icx_opt_name_suffix], \
                             "clang": [clang_opt_name_prefix, clang_opt_name_suffix], \
                             "dpcpp": [dpcpp_gpu_opt_name_prefix, dpcpp_gpu_opt_name_suffix]}

blame_test_makefile_name = "Blame_Makefile"

###############################################################################


def get_next_step(start, end, current, fail_flag):
    if fail_flag:
        next_start = start
        next_current = (current - start) // 2 + start
        next_end = current
    else:
        next_start = current
        next_current = (end - current) // 2 + current
        next_end = end
    return next_start, next_end, next_current


def dump_exec_output(msg, ret_code, output, err_output, time_expired, num):
    common.log_msg(logging.DEBUG, msg + " (process " + str(num) + ")")
    common.log_msg(logging.DEBUG, "Ret code: " + str(ret_code) + " | process " + str(num))
    common.log_msg(logging.DEBUG, "Time exp: " + str(time_expired) + " | process " + str(num))
    common.log_msg(logging.DEBUG, "Output: " + str(output, "utf-8") + " | process " + str(num))
    common.log_msg(logging.DEBUG, "Err output: " + str(err_output, "utf-8") + " | process " + str(num))


def execute_blame_phase(valid_res, fail_target, inject_str, num, phase_num):
    gen_test_makefile.gen_makefile(
            out_file_name = blame_test_makefile_name,
            force = True,
            config_file = None,
            only_target = fail_target,
            inject_blame_opt = inject_str + "-1" if fail_target.specs.name != "dpcpp" else None,
            inject_blame_env = inject_str + "1" if fail_target.specs.name == "dpcpp" else None)
    ret_code, output, err_output, time_expired, elapsed_time = \
        common.run_cmd(["make", "-f", blame_test_makefile_name, fail_target.name], run_gen.compiler_timeout, num)
    if fail_target.specs.name == "dpcpp":
        ret_code, output, err_output, time_expired, elapsed_time = \
            common.run_cmd(["make", "-f", blame_test_makefile_name, "run_" + fail_target.name], run_gen.compiler_timeout, num)

    opt_num_regex = re.compile(compilers_blame_patterns[fail_target.specs.name][phase_num])
    try:
        if fail_target.specs.name == "dpcpp":
            max_opt_num = 250
        else:
            matches = opt_num_regex.findall(str(err_output, "utf-8"))
            # Some icc phases may not support going to phase "2", i.e. drilling down to num_case level,
            # in this case we are done.
            if phase_num == 2 and not matches:
                return str(-1)
            max_opt_num_str = matches[-1]
            remove_brackets_pattern = re.compile("\d+")
            max_opt_num = int(remove_brackets_pattern.findall(max_opt_num_str)[-1])
            common.log_msg(logging.DEBUG, "Max opt num (process " + str(num) + "): " + str(max_opt_num))
    except IndexError:
        common.log_msg(logging.ERROR, "Can't decode max opt number using \"" + compilers_blame_patterns[fail_target.specs.name][phase_num]
                       + "\" regexp (phase " + str(phase_num) + ") in the following output:\n" + str(err_output, "utf-8")
                       + " (process " + str(num) + "): ")
        raise

    start_opt = 0
    end_opt = max_opt_num
    cur_opt = max_opt_num
    failed_flag = True
    time_to_finish = False
    while not time_to_finish:
        start_opt, end_opt, cur_opt = get_next_step(start_opt, end_opt, cur_opt, failed_flag)
        common.log_msg(logging.DEBUG, "Previous failed (process " + str(num) + "): " + str(failed_flag))
        failed_flag = False
        eff = ((start_opt + 1) >= cur_opt)  # Earliest fail was found

        common.log_msg(logging.DEBUG, "Trying opt (process " + str(num) + "): " + str(start_opt) + "/" + str(cur_opt) + "/" + str(end_opt))
        gen_test_makefile.gen_makefile(
                out_file_name = blame_test_makefile_name,
                force = True,
                config_file = None,
                only_target = fail_target,
                inject_blame_opt = inject_str + str(cur_opt) if fail_target.specs.name != "dpcpp" else None,
                inject_blame_env = inject_str + str(cur_opt) if fail_target.specs.name == "dpcpp" else None)
        ret_code, output, err_output, time_expired, elapsed_time = \
            common.run_cmd(["make", "-f", blame_test_makefile_name, fail_target.name], run_gen.compiler_timeout, num)
        if time_expired or ret_code != 0:
            dump_exec_output("Compilation failed", ret_code, output, err_output, time_expired, num)
            failed_flag = True
            if not eff:
                continue
            else:
                break

        ret_code, output, err_output, time_expired, elapsed_time = \
            common.run_cmd(["make", "-f", blame_test_makefile_name, "run_" + fail_target.name], run_gen.run_timeout, num)
        if time_expired or ret_code != 0:
            dump_exec_output("Execution failed", ret_code, output, err_output, time_expired, num)
            failed_flag = True
            if not eff:
                continue
            else:
                break

        if str(output, "utf-8") != valid_res:
            common.log_msg(logging.DEBUG, "Output differs (process " + str(num) + "): " + str(output, "utf-8") + " vs " + valid_res + " (expected)")
            failed_flag = True
            if not eff:
                continue
            else:
                break

        time_to_finish = (eff and failed_flag) or (eff and not failed_flag and (cur_opt == (end_opt - 1)))
        common.log_msg(logging.DEBUG, "Time to finish (process " + str(num) + "): " + str(time_to_finish))

    if not failed_flag:
        common.log_msg(logging.DEBUG, "Swapping current and end opt (process " + str(num) + ")")
        cur_opt = end_opt

    common.log_msg(logging.DEBUG, "Finished blame phase, result: " + str(inject_str) + str(cur_opt) + " (process " + str(num) + ")")

    return cur_opt


def blame(fail_dir, valid_res, fail_target, out_dir, lock, num, inplace):
    blame_str = ""
    stdout = stderr = b""
    if not re.search("-O0", fail_target.args):
        blame_opts = compilers_blame_opts[fail_target.specs.name]
        phase_num = 0
        blame_phase_num = 0
        # Do blaming
        try:
            for i in blame_opts:
                blame_str += i
                blame_phase_num = execute_blame_phase(valid_res, fail_target, blame_str, num, phase_num)
                if fail_target.specs.name == "dpcpp":
                    # Special case because triaging mechanism is different and there's only one level of triaging.
                    blame_str += str(blame_phase_num-1)
                else:
                    blame_str += str(blame_phase_num)
                blame_str += " "
                phase_num += 1
        except:
            common.log_msg(logging.ERROR, "Something went wrong while executing blame_opt.py on " + str(fail_dir))
            return False

        # Wrap up results
        gen_test_makefile.gen_makefile(
                out_file_name = blame_test_makefile_name,
                force = True,
                config_file = None,
                only_target = fail_target,
                inject_blame_opt = blame_str if fail_target.specs.name != "dpcpp" else None,
                inject_blame_env = blame_str if fail_target.specs.name == "dpcpp" else None)
        ret_code, stdout, stderr, time_expired, elapsed_time = \
            common.run_cmd(["make", "-f", blame_test_makefile_name, fail_target.name], run_gen.compiler_timeout, num)
        if fail_target.specs.name == "dpcpp":
            ret_code, stdout, stderr, time_expired, elapsed_time = \
                common.run_cmd(["make", "-f", blame_test_makefile_name, "run_" + fail_target.name], run_gen.compiler_timeout, num)

        if fail_target.specs.name != "dpcpp":
            opt_name_pattern = re.compile(compilers_opt_name_cutter[fail_target.specs.name][0] + ".*" +
                                          compilers_opt_name_cutter[fail_target.specs.name][1])
            opt_name = opt_name_pattern.findall(str(stderr, "utf-8"))[-1]
            opt_name = re.sub(compilers_opt_name_cutter[fail_target.specs.name][0], "", opt_name)
            opt_name = re.sub(compilers_opt_name_cutter[fail_target.specs.name][1], "", opt_name)
            real_opt_name = opt_name
            opt_name = opt_name.replace(" ", "_")
        else:
            if blame_phase_num == 1:
                # It's special case for DPC++. 1 means that triaging failed, no specific phase can be blamed.
                real_opt_name = opt_name = "FailedToBlame"
            else:
                opt_name_pattern = re.compile(compilers_opt_name_cutter[fail_target.specs.name][0] + ".*" +
                                              compilers_opt_name_cutter[fail_target.specs.name][1])
                opt_name = opt_name_pattern.findall(str(stderr, "utf-8"))[0]
                opt_name = re.sub(compilers_opt_name_cutter[fail_target.specs.name][0], "", opt_name)
                opt_name = re.sub(compilers_opt_name_cutter[fail_target.specs.name][1], "", opt_name)
                real_opt_name = opt_name
                opt_name = opt_name.replace(" ", "_")
    else:
        real_opt_name = opt_name = "O0_bug"

    common.run_cmd(["make", "-f", blame_test_makefile_name, "clean"], run_gen.compiler_timeout, num)

    seed_dir = os.path.basename(os.path.normpath(fail_dir))
    # Create log files in different places depending on "inplace" switch.
    if not inplace:
        full_out_path = os.path.join(os.path.join(out_dir, opt_name), seed_dir)
        common.copy_test_to_out(fail_dir, full_out_path, lock)
    else:
        full_out_path = "."

    # Write to log
    with open(os.path.join(full_out_path, "log.txt"), "a") as log_file:
        log_file.write("\nBlaming for " + fail_target.name + " optset was done.\n")
        log_file.write("Optimization to blame: " + real_opt_name + "\n")
        log_file.write("Blame opts: " + blame_str + "\n\n")
        log_file.write("Details of blaming run:\n")
        log_file.write("=== Compiler log ==================================================\n")
        log_file.write(str(stdout, "utf-8"))
        log_file.write("=== Compiler err ==================================================\n")
        log_file.write(str(stderr, "utf-8"))
        log_file.write("=== Compiler end ==================================================\n")

    common.log_msg(logging.DEBUG, "Done blaming")

    # Inplace mode require blaming string to be communicated back to the caller
    if not inplace:
        return True
    else:
        return real_opt_name


def prepare_env_and_blame(fail_dir, valid_res, fail_target, out_dir, lock, num, inplace=False):
    common.log_msg(logging.DEBUG, "Blaming target: " + fail_target.name + " | " + fail_target.specs.name)
    os.chdir(fail_dir)
    if fail_target.specs.name not in compilers_blame_opts:
        common.log_msg(logging.DEBUG, "We can't blame " + fail_target.name + " (process " + str(num) + ")")
        return False
    return blame(fail_dir, valid_res, fail_target, out_dir, lock, num, inplace)
