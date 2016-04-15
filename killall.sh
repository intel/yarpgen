#!/bin/bash
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


IFS=$'\n'       # make newlines the only separator
PIDS=""
for str in `ps -ef | grep run_gen.py` 
do
    echo "$str: $(echo $str | grep -oP 'amitrokh  \K\w+' )"
    PIDS="$(echo $str | grep -oP 'amitrokh  \K\w+' ) $PIDS"
done

echo $PIDS


IFS=$' '
pkill python
pkill run_gen.py
pkill out
