#!/usr/bin/python
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


import re, math


f = open("log.txt", "ro")
fstr = f.read()
f.close()



#fstr = fstr[0:80000]
#print fstr

#print re.findall("Job #(?P<job>\d+) \(/\*", fstr, flags=0)


CMPL_RE_STR = "Job #(?P<job>\d+) \(/\*SEED (?P<seed>\d+)\*/\n\) COMPILE - make -f (?P<makepath>[/\w]+) EXECUTABLE=out_\d+_(?P<target>\w+) (?P<target2>\w+): (?P<time>\d+.\d+)"
RNTM_RE_STR = "Job #(?P<job>\d+) \(/\*SEED (?P<seed>\d+)\*/\n\) RUN - out_\d+_(?P<target>\w+): (?P<time>\d+.\d+)"
RNTM_SDE_RE_STR = "Job #(?P<job>\d+) \(/\*SEED (?P<seed>\d+)\*/\n\) RUN - \['bash', '-c', 'sde -(?P<sde_target>\w+) -- ./out_\d+_(?P<target>\w+)'\]: (?P<time>\d+.\d+)"

class TargetDB:
    def __init__(self):
        self.targets = {}

    def add(self, target, time):
        if (target not in self.targets.keys()):
            self.targets[target] = [1, float(time), math.log(float(time)), float(time), [float(time)]]
            return

        self.targets[target][0] += 1
        self.targets[target][1] += float(time)
        self.targets[target][2] += math.log(float(time))
        self.targets[target][3] *= float(time)
        self.targets[target][4].append(float(time))

    def dump(self):
        for k in self.targets.keys():
            print (str(k) + ": " + str(self.targets[k][0]) + " "
                                 + str(self.targets[k][1]) + " "
                                 + str(self.targets[k][2]) + " "
                                 + str(self.targets[k][1]/float(self.targets[k][0])) + " "
                                 + str(math.exp(self.targets[k][2]/float(self.targets[k][0]))) + " "
                                 + str(math.pow(self.targets[k][3], 1.0/float(self.targets[k][0]))))

    def dump_to_file(self, f_name):
        f = open(f_name, 'w')
        for k in self.targets.keys():
            f.write(k + '\n\n')
            for t in self.targets[k][4]:
                f.write(str(t) + '\n')
        f.close()

class SEED:
    def __init__(self):
        self.targets_c = {}
        self.targets_r = {}

    def add_c(self, target, time):
        self.targets_c[target] = float(time)

    def add_r(self, target, time):
        self.targets_r[target] = float(time)


cdb = TargetDB()
for cmpl in re.finditer(CMPL_RE_STR, fstr, flags=0):
    print cmpl.groupdict()
    cdb.add(cmpl.group('target'), cmpl.group('time'))
cdb.dump()

rdb = TargetDB()
for cmpl in re.finditer(RNTM_RE_STR, fstr, flags=0):
    print cmpl.groupdict()
    rdb.add(cmpl.group('target'), cmpl.group('time'))

sderdb = TargetDB()
for cmpl in re.finditer(RNTM_SDE_RE_STR, fstr, flags=0):
    print cmpl.groupdict()
    sderdb.add(cmpl.group('target'), cmpl.group('time'))


print "\nCompile Time Dump:"
cdb.dump()

print "\nRun Time Dump:"
rdb.dump()
#rdb.dump_to_file("foo.txt")

print "\nSDE Run Time Dump:"
sderdb.dump()
