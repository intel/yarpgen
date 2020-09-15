#!/usr/bin/python3
###############################################################################
#
# Copyright (c) 2020, Intel Corporation
# Copyright (c) 2020, University of Utah
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
Script that verifies UB substitution rules, which are used in YARP Generator
"""
###############################################################################

from z3 import *

BITWIDTH = 64

INT_MIN = BitVecVal(1 << BITWIDTH - 1, BITWIDTH)
INT_MAX = BitVecVal((1 << BITWIDTH - 1) - 1, BITWIDTH)
INT_MIN_AS_LONG = SignExt(BITWIDTH, INT_MIN)
INT_MAX_AS_LONG = SignExt(BITWIDTH, INT_MAX)

def checkit(name, cond, repl):
    s = Solver()
    s.push()
    s.add(cond)
    s.add(repl)
    if s.check() == sat:
        m = s.model()
        print(name + " : OOPS!!! a = " +
              str(m.evaluate(a)) + " (" + str(m.evaluate(a).as_signed_long()) + ")" +
              " b = " +
              str(m.evaluate(b)) + " (" + str(m.evaluate(b).as_signed_long()) + ")")
    else:
        print(name + " : OK")
    s.pop()

a, b = BitVecs('a b', BITWIDTH)
a_as_long = SignExt(BITWIDTH, a)
b_as_long = SignExt(BITWIDTH, b)

def addUB(a, b):
    return Or((a_as_long + b_as_long) > INT_MAX_AS_LONG,
              (a_as_long + b_as_long) < INT_MIN_AS_LONG)

def subUB(a, b):
    return Or((a_as_long - b_as_long) > INT_MAX_AS_LONG,
              (a_as_long - b_as_long) < INT_MIN_AS_LONG)

def mulUB1(a, b):
    return And(a == INT_MIN, b == -1)

def mulUB2(a, b):
    return Or((a_as_long * b_as_long) > INT_MAX_AS_LONG,
              (a_as_long * b_as_long) < INT_MIN_AS_LONG)

def mulUB(a, b):
    return Or(mulUB1(a, b), mulUB2(a, b))

def sdivUB1(a, b):
    return And(a == INT_MIN, b == -1)

def sdivUB2(a, b):
    return b == 0

def sdivUB(a, b):
    return Or(sdivUB1(a, b), sdivUB2(a, b))

def udivUB(a, b):
    return b == 0

def smodUB1(a, b):
    return b == 0

def smodUB2(a, b):
    return And(a == INT_MIN, b == -1)

def smodUB(a, b):
    return Or(smodUB1(a, b), smodUB2(a, b))

def umodUB(a, b):
    return b == 0


checkit("a + b -> a - b", addUB(a, b), subUB(a, b))
checkit("a + b -> a + b", Not(addUB(a, b)), addUB(a, b))

checkit("a - b -> a + b", subUB(a, b), addUB(a, b))
checkit("a - b -> a - b", Not(subUB(a, b)), subUB(a, b))

checkit("a * b -> a - b", mulUB1(a, b), subUB(a, b))
checkit("a * b -> a / b", And(Not(mulUB1(a, b)), mulUB2(a, b)), sdivUB(a, b))
checkit("a * b -> a * b", Not(mulUB(a, b)), mulUB(a, b))

checkit("a /u b -> a * b", udivUB(a, b), mulUB(a, b))
checkit("a /u b -> a /u b", Not(udivUB(a, b)), udivUB(a, b))
checkit("a /s b -> a - b", sdivUB1(a, b), subUB(a, b))
checkit("a /s b -> a * b", sdivUB2(a, b), mulUB(a, b))
checkit("a /s b -> a /s b", Not(sdivUB(a, b)), sdivUB(a, b))

checkit("a %/u b -> a * b", umodUB(a, b), mulUB(a, b))
checkit("a %/u b -> a %/u b", Not(umodUB(a, b)), umodUB(a, b))
checkit("a %/s b -> a * b", smodUB1(a, b), mulUB(a, b))
checkit("a %/s b -> a - b", smodUB2(a, b), subUB(a, b))
checkit("a %/s b -> a %/s b", Not(smodUB(a, b)), smodUB(a, b))

