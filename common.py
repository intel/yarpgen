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
File for common data and functions, which are used in scripts
"""
###############################################################################

import datetime
import errno
import logging
import os
import shutil
import subprocess
import sys

# $YARPGEN_HOME environment variable should be set to YARP Generator directory
yarpgen_home = os.environ["YARPGEN_HOME"] if "YARPGEN_HOME" in os.environ else os.getcwd()
yarpgen_version_str = ""

main_logger_name = "main_logger"
main_logger = None
__duplicate_err_to_stderr__ = False

stat_logger_name = "stat_logger"
stat_logger = None


def print_and_exit(msg):
    log_msg(logging.ERROR, msg)
    exit(-1)


def setup_logger(log_file, log_level):
    global main_logger
    main_logger = logging.getLogger(main_logger_name)
    formatter = logging.Formatter("%(asctime)s [%(levelname)-5.5s]  %(message)s")
    main_logger.setLevel(log_level)

    if log_file is not None:
        file_handler = logging.FileHandler(log_file)
        file_handler.setFormatter(formatter)
        main_logger.addHandler(file_handler)
        global __duplicate_err_to_stderr__
        __duplicate_err_to_stderr__ = True
    else:
        stream_handler = logging.StreamHandler()
        stream_handler.setFormatter(formatter)
        main_logger.addHandler(stream_handler)


def wrap_log_file(log_file, default_log_file):
    if log_file == default_log_file:
        log_file = log_file.replace(".log", "")
        return log_file + "_" + datetime.datetime.now().strftime('%Y_%m_%d_%H_%M_%S') + ".log"
    else:
        return log_file


def log_msg(log_level, message, forced_duplication = False):
    global main_logger
    main_logger.log(log_level, message)
    if __duplicate_err_to_stderr__ and (log_level == logging.ERROR or forced_duplication):
        sys.stderr.write(str(message) + "\n")
        sys.stderr.flush()


class StatisticsFileHandler(logging.FileHandler):
    def emit(self, record):
        if self.stream is None:
            self.stream = self._open()
        logging.StreamHandler.emit(self, record)
        self.close()


def setup_stat_logger(log_file):
    global stat_logger
    stat_logger = logging.getLogger(stat_logger_name)
    if log_file is not None:
        stat_handler = StatisticsFileHandler(filename=log_file, mode="w", delay=True)
        stat_logger.addHandler(stat_handler)
    stat_logger.setLevel(logging.INFO)


def check_python_version():
    if sys.version_info < (3, 3):
        print_and_exit("This script requires at least python 3.3.")


def check_and_open_file(file_name, mode):
    norm_file_name = os.path.abspath(file_name)
    if not os.path.isfile(norm_file_name):
        print_and_exit("File " + norm_file_name + " doesn't exist and can't be opened")
    return open(norm_file_name, mode)


def check_and_copy(src, dst):
    if not isinstance(src, str) or not isinstance(dst, str):
        print_and_exit("Src and dst should be strings")
    norm_src = os.path.abspath(src)
    norm_dst = os.path.abspath(dst)
    if os.path.exists(norm_src):
        log_msg(logging.DEBUG, "Copying " + norm_src + " to " + norm_dst)
        shutil.copy(norm_src, norm_dst)
    else:
        print_and_exit("File " + norm_src + " wasn't found")


def copy_test_to_out(test_dir, out_dir, lock):
    log_msg(logging.DEBUG, "Copying " + test_dir + " to " + out_dir)
    lock.acquire()
    try:
        shutil.copytree(test_dir, out_dir)
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise
    finally:
        lock.release()


def check_if_dir_exists(directory):
    norm_dir = os.path.abspath(directory)
    if not os.path.exists(norm_dir) or not os.path.isdir(norm_dir):
        return False
    return True


def check_dir_and_create(directory):
    norm_dir = os.path.abspath(directory)
    if not os.path.exists(norm_dir):
        log_msg(logging.DEBUG, "Creating '" + str(norm_dir) + "' directory")
        os.makedirs(norm_dir)
    elif not os.path.isdir(norm_dir):
        print_and_exit("Can't use '" + norm_dir + "' directory")


def run_cmd(cmd, time_out=None, num=-1):
    is_time_expired = False
    start_time = os.times()
    with subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
        try:
            log_msg_str = "Running " + str(cmd)
            if num != -1:
                log_msg_str += " in process " + str(num)
            log_msg(logging.DEBUG, log_msg_str)
            output, err_output = process.communicate(timeout=time_out)
            ret_code = process.poll()
        except subprocess.TimeoutExpired:
            process.kill()
            log_msg(logging.DEBUG, str(cmd) + " failed")
            output, err_output = process.communicate()
            is_time_expired = True
            ret_code = None
        except:
            log_msg(logging.ERROR, str(cmd) + " failed: unknown exception")
            process.kill()
            process.wait()
            raise
    end_time = os.times()
    elapsed_time = end_time.children_user - start_time.children_user + \
                   end_time.children_system - start_time.children_system
    return ret_code, output, err_output, is_time_expired, elapsed_time


def if_exec_exist(program):
    def is_exe(file_path):
        return os.path.isfile(file_path) and os.access(file_path, os.X_OK)

    log_msg(logging.DEBUG, "Checking if " + str(program) + " exists")
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
    log_msg(logging.DEBUG, "Exec wasn't found")
    return False

def clean_dir(path):
    for root, dirs, files in os.walk(path, topdown=False):
        for name in files:
            os.remove(os.path.join(root, name))
        for name in dirs:
            os.rmdir(os.path.join(root, name))
