#!/bin/bash
###############################################################################
#
# Copyright (c) 2015-2017, Intel Corporation
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

find /tmp -name "check*.cpp" -type f -mmin +60 -delete
find /tmp -name "driver*.cpp" -type f -mmin +60 -delete
find /tmp -name "driver*.o" -type f -mmin +60 -delete
find /tmp -name "func*.cpp" -type f -mmin +60 -delete
find /tmp -name "func*.sh" -type f -mmin +60 -delete
find /tmp -name "hash*.cpp" -type f -mmin +60 -delete
find /tmp -name "init*.cpp" -type f -mmin +60 -delete
find /tmp -name "init*.sh" -type f -mmin +60 -delete
find /tmp -name "iccdash*" -type f -mmin +60 -delete
find /tmp -name "iccdummy*" -type f -mmin +60 -delete
find /tmp -name "iccgccdash*" -type f -mmin +60 -delete
find /tmp -name "icclibgcc*" -type f -mmin +60 -delete
echo "tmp_cleaner was run" >> cleaner.log
