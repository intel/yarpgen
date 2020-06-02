#!/bin/bash
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

current_user=`whoami`
find /tmp -user $current_user -name "check*.cpp" -type f -mmin +60 -delete
find /tmp -user $current_user -name "driver*.cpp" -type f -mmin +60 -delete
find /tmp -user $current_user -name "driver*.o" -type f -mmin +60 -delete
find /tmp -user $current_user -name "func*.cpp" -type f -mmin +60 -delete
find /tmp -user $current_user -name "func*.sh" -type f -mmin +60 -delete
find /tmp -user $current_user -name "f-*.sh" -type f -mmin +60 -delete
find /tmp -user $current_user -name "hash*.cpp" -type f -mmin +60 -delete
find /tmp -user $current_user -name "init*.cpp" -type f -mmin +60 -delete
find /tmp -user $current_user -name "init*.sh" -type f -mmin +60 -delete
find /tmp -user $current_user -name "iccdash*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "iccdummy*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "iccgccdash*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "icclibgcc*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "icpc*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "data.*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "creduce*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "tmp*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "cc*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "symbolizer*" -type f -mmin +60 -delete
find /tmp -user $current_user -name "??????????" -type f -mmin +60 -delete
echo "tmp_cleaner was run" >> cleaner.log
