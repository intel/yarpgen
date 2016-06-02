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


import sys, os, shutil

DIR = "./test/result"
rate = 0.3

def levenshteinDistance(s1, s2):
    if len(s1) > len(s2):
        s1, s2 = s2, s1

    distances = range(len(s1) + 1)
    for i2, c2 in enumerate(s2):
        distances_ = [i2+1]
        for i1, c1 in enumerate(s1):
            if c1 == c2:
                distances_.append(distances[i1])
            else:
                distances_.append(1 + min((distances[i1], distances[i1 + 1], distances_[-1])))
        distances = distances_
    return distances[-1]


groups = {}
candidates = []


def get_sample(f):
    if (not "log.txt" in os.listdir(f)):
        print "No log.txt in " + f + "!"
        print "Please, remove faulty " + f + " test and relaunch."
        sys.exit()

    log_txt = open(f + "/log.txt", 'r')
    sample = log_txt.read()
    log_txt.close()
    return sample


def handle_group(f):
    files = os.listdir(f)
    target = ""
    for t_ in files:
        if ("S_" in t_):
            target = t_
            break
    if (not "S_" in target):
        print "No test cases in target " + f + "!"
        print "Please, remove faulty " + f + " group and relaunch."
        sys.exit()

    groups[f] = get_sample(f+"/"+target)


def new_group(t_, sample):
    name = DIR+"/group_"
    id = 0
    while ((name+str(id)) in groups.keys()):
        id = id + 1

    name = name + str(id)
    print "Creating new group " + name
    os.mkdir(name)
    shutil.move(t_, name)
    groups[name] = sample
    print "Groups: ", groups.keys()


for f in os.listdir(DIR):
    if ("S_" in f):
        candidates.append(DIR+"/"+f)
    if ("group_" in f):
        handle_group(DIR+"/"+f)


print "Found candidates:", candidates
print "Found groups:", groups.keys()


for f in candidates:
    sample = get_sample(f)
    processed = False
    for g in groups.keys():
        g_sample = groups[g]
        print "Processing: " + str(g) + "; sizes are: " + str(len(sample)) + ", " + str(len(g_sample))
        
        distance = abs(len(sample) - len(g_sample))
        error = float(2*distance)/float(len(sample)+len(g_sample))

        if (error < rate):
            if ((len(sample) > 3000) or (len(g_sample) > 3000)):
                continue

            distance = levenshteinDistance(sample, g_sample)
            error = float(2*distance)/float(len(sample)+len(g_sample))
        
        print ("Distance between " + f + " and " + g + " is " + str(distance) + 
               " error rate is: " + str(error))

        if (error < rate) and (error > rate/2):
            processed = True
            print "(DIY): Dunno how to handle " + f

        if (error < rate/2):
            if (processed):
                print "Already classified! Maybe error?: " + f

            processed = True
            print " - Adding to group " + g
            shutil.move(f, g)

    if (processed):
        continue

    new_group(f, sample)

