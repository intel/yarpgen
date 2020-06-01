[![TravisCI build status (Linux and Mac)](https://travis-ci.org/intel/yarpgen.svg?branch=master)](https://travis-ci.org/intel/yarpgen)
[![Appveyor build status (Windows and Ubuntu)](https://ci.appveyor.com/api/projects/status/meuyl409mtd4cljb/branch/master?svg=true)](https://ci.appveyor.com/project/webmasterintel/yarpgen/branch/master)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](https://github.com/intel/yarpgen/blob/master/LICENSE.txt)

Yet Another Random Program Generator
====================================

``yarpgen`` is a random C/C++ program generator, which produces correct runnable C/C++ programs. The generator is specifically designed to trigger compiler optimization bugs and is intended for compiler testing.

A generated random program is guaranteed to be statically and dynamically correct C/C++ program. This implies no undefined behavior, but allows for implementation defined behavior.

Each generated program consists of several files and after being compiled and run produces a decimal number, which is hash of all program global variable values. This number is supposed to be the same for all compilers and optimization levels. If the output differs for different compilers and/or optimization levels, you should have encountered a compiler bug.

Several techniques are used to increase the probability of triggering compiler bugs:

* all variable types, alignments and values / value ranges are known at the time of generation, which allows the accurate detection of undefined behavior and provides maximum variability of produced code.
* the generation is random, but it is guided by a number of policies, which increase the likelihood of applying optimizations to the generated code.
* in some cases a high level model of computations with known properties is generated first and then augmented with other random computations. This ensures that the code is "meaningful" and is more similar to code written by a human being.

Compiler bugs found by YARP Generator
-------------------------------------

``yarpgen`` managed to find more than 140 bugs in ``gcc`` and ``clang`` and a comparable number of bugs in proprietary compilers. For the list of the bugs in ``gcc`` and ``clang`` see bugs.rst.

If you have used ``yarpgen`` to find bugs in open source compilers, please update bugs.rst. We are also excited to hear about your experience using it with proprietary compilers and other tools you might want to validate.

Building and running
--------------------

Building ``yarpgen`` is trivial.  All you have to do is invoke "make".

To run ``yarpgen`` we recommend using ``run_gen.py`` script, which will run the generator for you on a number of available compilers with a set of pre-defined options. Feel free to hack test_set.txt to add or remove compiler options.

The script will run several compilers with several compiler options and run executables to compare the output results. If the results mismatch, the test program will be saved in "results" folder for your analysis.

Also you may want to test compilers for future hardware, which is not available to you at the moment. The standard way to do that is to download the [Intel® Software Development Emulator](http://www.intel.com/software/sde). ``run_gen.py`` assumes that it is available in your PATH.

Contacts
--------

To contact authors, ask questions, or leave your feedback please use Github [issues](https://github.com/intel/yarpgen/issues) or reach out directly through contacts available in Github profiles.

People
------

* Dmitry Babokin
* John Regehr
* Vsevolod Livinskiy
