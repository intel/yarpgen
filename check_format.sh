#!/bin/bash
###############################################################################
#
# Copyright (c) 2018-2020, Intel Corporation
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
EXIT_CODE=0
echo "\
############################################################################
Checking formatting of modified files. It is expected that the files were
formatted with clang-format 10.0.0. It is also expected that clang-format
version 10.0.0 is used for the check. Otherwise the result can ne unexpected.
############################################################################"

CLANG_FORMAT="clang-format"
[[ ! -z $1 ]] && CLANG_FORMAT=$1
REQUIRED_VERSION="10.0.0"
VERSION_STRING="clang-format version $REQUIRED_VERSION.*"
CURRENT_VERSION="$($CLANG_FORMAT --version)"
if ! [[ $CURRENT_VERSION =~ $VERSION_STRING ]] ; then
    echo WARNING: clang-format version $REQUIRED_VERSION is required but $CURRENT_VERSION is used.
    echo The results can be unexpected.
fi

# Check all source files.
FILES=`ls src/*.{h,cpp}`
for FILE in $FILES; do
    $CLANG_FORMAT $FILE | cmp  $FILE >/dev/null
    if [ $? -ne 0 ]; then
        echo "[!] INCORRECT FORMATTING! $FILE" >&2
            EXIT_CODE=1
        fi
    done
if [ $EXIT_CODE -eq 0 ]; then 
    echo "No formatting issues found"
fi
exit $EXIT_CODE
