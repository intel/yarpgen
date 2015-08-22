#/bin/bash
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


cd ./result

while [ "$?" -eq "0" ]
do
    rm test.cpp test.s icc-no-opt.log icc-opt.log test.optrpt
    ../a.out 1> test.cpp

    icpc test.cpp -S -O0 -restrict -std=c++11
    icpc test.s driver.cpp -O0 -restrict -o out -std=c++11
    ./out > icc-no-opt.log
    icpc test.cpp -S -O3 -vec-report=7 -restrict -std=c++11
    icpc test.s driver.cpp -O3 -restrict -o out -std=c++11
    ./out > icc-opt.log

    clang++ test.cpp -S -O0 -std=c++11
    clang++ test.s driver.cpp -O0 -o out -std=c++11
    ./out > clang-no-opt.log
    clang++ test.cpp -S -O3 -std=c++11
    clang++ test.s driver.cpp -O3 -o out -std=c++11
    ./out > clang-opt.log

#    cat test.optrpt
    diff icc-opt.log icc-no-opt.log
    diff clang-opt.log clang-no-opt.log
    diff icc-opt.log clang-opt.log

    echo $?
done
