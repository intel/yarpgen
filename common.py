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

import datetime
import logging
import os
import shutil
import subprocess
import sys

# $YARPGEN_HOME environment variable should be set to YARP Generator directory
yarpgen_home = os.environ["YARPGEN_HOME"] if "YARPGEN_HOME" in os.environ else os.getcwd()
yarpgen_version = ""

stderr_logger_name = "stderr_logger"
stderr_logger = logging.getLogger(stderr_logger_name)

file_logger_name = "file_logger"
file_logger = logging.getLogger(file_logger_name)

stat_logger_name = "stat_logger"
stat_logger = logging.getLogger(stat_logger_name)


def setup_logger (logger_name = "", log_file = "", file_mode = "a", log_level = logging.ERROR, write_to_stderr = False):
    l = logging.getLogger(logger_name)
    formatter = logging.Formatter("%(asctime)s [%(levelname)-5.5s]  %(message)s")
    l.setLevel(log_level)

    if (log_file != ""):
        fileHandler = logging.FileHandler(log_file, mode = file_mode)
        fileHandler.setFormatter(formatter)
        l.addHandler(fileHandler)

    if (write_to_stderr):
        streamHandler = logging.StreamHandler()
        streamHandler.setFormatter(formatter)
        l.addHandler(streamHandler)


def wrap_log_file (log_file, default_log_file):
    if (log_file == default_log_file):
        return log_file + "_" + datetime.datetime.now().strftime('%Y_%m_%d_%H_%M_%S')
    else:
        return log_file


def log_msg (log_level, message):
    stderr_logger.log(log_level, message)
    file_logger.log(log_level, message)


def print_and_exit (msg):
    log_msg(logging.ERROR, msg)
    exit (-1)


def check_python_version():
    if sys.version_info < (3, 3):
        print_and_exit("This script requires at least python 3.3.")


def check_and_open_file(file_name, mode):
    norm_file_name = os.path.abspath(file_name)
    if (not os.path.isfile(norm_file_name)):
        print_and_exit("File " + norm_file_name + " doesn't exist and can't be opened")
    return open(norm_file_name, mode)


def check_and_copy (src, dst):
    if not isinstance(src, str) or not isinstance(dst, str):
        print_and_exit("Src and dst should be strings")
    norm_src = os.path.abspath(src)
    norm_dst = os.path.abspath(dst)
    if os.path.exists(norm_src):
        log_msg(logging.DEBUG, "Copying " + norm_src + " to " + norm_dst)
        shutil.copy(norm_src, norm_dst)
    else:
        print_and_exit("File " + norm_src + " wasn't found")


def check_dir_and_create (directory):
    norm_dir = os.path.abspath(directory)
    if (not os.path.exists(norm_dir)):
        log_msg(logging.DEBUG, "Creating '" + str(norm_dir) + "' directory")
        os.makedirs(norm_dir)
    elif (not os.path.isdir(norm_dir)):
        print_and_exit("Can't use '" + norm_dir + "' directory")


def run_cmd (cmd, time_out = None, num = -1):
    time_expired = False
    ret_code = 0
    with subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE) as process:
        try:
            log_msg_str = "Running " + str(cmd)
            if (num != -1):
                log_msg_str += " in process " + str(num)
            log_msg(logging.DEBUG, log_msg_str)
            output, err_output = process.communicate(timeout = time_out)
            ret_code = process.poll()
        except subprocess.TimeoutExpired:
            process.kill()
            log_msg(logging.DEBUG, str(cmd) + " failed")
            output, err_output = process.communicate()
            time_expired = True
            ret_code = None
        except:
            log_msg(logging.ERROR, str(cmd) + " failed: unknown exception")
            process.kill()
            process.wait()
            raise
    return ret_code, output, err_output, time_expired


def if_exec_exist (program):
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    log_msg(logging.DEBUG,"Checking if " + str(program) + " exists")
    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            log_msg(logging.DEBUG, "Exec " + program + " was found at " + program)
            return True
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                log_msg(logging.DEBUG, "Exec " + program + " was found at " + exe_file)
                return True
    log_msg(logging.DEBUG,"Exec wasn't found")
    return False


def remove_file_if_exists(file_name):
    if (os.path.isfile(file_name)):
        os.remove(file_name)


def parse_time_log(file_name):
    time_log_file = check_and_open_file(file_name, "r")
    time_results_log = time_log_file.readlines()
    time_log_file.close()

    duration = datetime.timedelta()
    for i in time_results_log:
        #TODO: time --quiet doesn't work, so we use this kludge
        if i.startswith("Command"):
            continue
        time_results = i.split()
        duration += datetime.timedelta(seconds = float(time_results[0]))
        duration += datetime.timedelta(seconds = float(time_results[1]))

    remove_file_if_exists(file_name)
    return duration
