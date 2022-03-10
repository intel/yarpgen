Yet Another Random Program Generator
====================================

[![TravisCI build status (Linux and Mac)](https://travis-ci.org/intel/yarpgen.svg?branch=main)](https://travis-ci.org/intel/yarpgen)
[![Appveyor build status (Windows and Ubuntu)](https://ci.appveyor.com/api/projects/status/meuyl409mtd4cljb/branch/main?svg=true)](https://ci.appveyor.com/project/webmasterintel/yarpgen/branch/main)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](https://github.com/intel/yarpgen/blob/main/LICENSE.txt)

``yarpgen`` is a random program generator, which produces correct runnable C/C++ and DPC++(this work is in an early stage) programs. The generator is specifically designed to trigger compiler optimization bugs and is intended for compiler testing.

A generated random program is guaranteed to be statically and dynamically correct program. This implies no undefined behavior, but allows for implementation defined behavior.

Currently, there exist two versions of YARPGen: one is designed to test loops (it is under development and lives in the main branch), and the other one is designed to test scalar optimizations (it lives in [v1 branch](https://github.com/intel/yarpgen/tree/v1)). More information about YARPGen and scalar version can be found in [this talk](https://www.youtube.com/watch?v=mb9aRoXnicE) and [this paper](papers/yarpgen-ooplsa-2020.pdf), published at the OOPSLA 2020 and received an ACM SIGPLAN distinguished paper award.

Each generated program consists of several files and after being compiled and run produces a decimal number, which is hash of all program global variable values. This number is supposed to be the same for all compilers and optimization levels. If the output differs for different compilers and/or optimization levels, you should have encountered a compiler bug.

Several techniques are used to increase the probability of triggering compiler bugs:

* all variable types, alignments and values / value ranges are known at the time of generation, which allows the accurate detection of undefined behavior and provides maximum variability of produced code.
* the generation is random, but it is guided by a number of policies, which increase the likelihood of applying optimizations to the generated code.
* in some cases a high level model of computations with known properties is generated first and then augmented with other random computations. This ensures that the code is "meaningful" and is more similar to code written by a human being.

Compiler bugs found by YARP Generator
-------------------------------------

``yarpgen`` managed to find more than 220 bugs in [``gcc``](https://gcc.gnu.org/), [``clang``](https://clang.llvm.org/), [``ispc``](https://ispc.github.io/), [``dpc++``](https://software.intel.com/content/www/us/en/develop/tools/oneapi/components/dpc-compiler.html), [``sde``](https://software.intel.com/content/www/us/en/develop/articles/intel-software-development-emulator.html), [`alive2`](https://github.com/AliveToolkit/alive2) and a comparable number of bugs in
proprietary compilers. For the list of the bugs in ``gcc``, ``clang``, ``ispc``, and ``alive2`` see [bugs.rst](bugs.rst).

If you have used ``yarpgen`` to find bugs in open source compilers, please update bugs.rst. We are also excited to hear about your experience using it with proprietary compilers and other tools you might want to validate.

Building and running
--------------------

Building ``yarpgen`` is trivial.  All you have to do is to use cmake:

```sh
mkdir build
cd build
cmake ..
make
```

To run ``yarpgen`` we recommend using [``run_gen.py``](scripts/run_gen.py) script, which will run the generator for you
on a number of available compilers with a set of pre-defined options. Feel free to hack
[``test_sets.txt``](scripts/test_sets.txt) to add or remove compiler options.

The script will run several compilers with several compiler options and run executables to compare the output results. If the results mismatch, the test program will be saved in "testing/results" folder for your analysis.

Also you may want to test compilers for future hardware, which is not available to you at the moment. The standard way to do that is to download the [Intel® Software Development Emulator](http://www.intel.com/software/sde). ``run_gen.py`` assumes that it is available in your ``$PATH``.

ISPC testing
------------

If you want to test [ISPC](https://ispc.github.io/), make sure that it is present in your path, as well as
[``ispc-proxy``](scripts/ispc-proxy) and [``ispc-disp``](scripts/ispc-disp). After that you can use
[``run_gen.py``](scripts/run_gen.py) as usual.

Contacts
--------

To contact authors, ask questions, or leave your feedback please use Github [issues](https://github.com/intel/yarpgen/issues) or reach out directly through contacts available in Github profiles.

People
------

* Dmitry Babokin
* John Regehr
* Vsevolod Livinskiy
