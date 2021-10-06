#!/bin/bash -e

if [[ -z "${YARPGEN_HOME}" ]]; then
    echo "Error! Environmental variable for YARPGEN_HOME is not set!"
    exit 127
fi

cd $YARPGEN_HOME
mkdir build
git remote -v > $YARPGEN_HOME/yarpgen_rev.txt
git log --format="%H" -n 1 >> $YARPGEN_HOME/yarpgen_rev.txt

# Pre-compilation steps
cd $YARPGEN_HOME/build
cmake -G "Ninja" $YARPGEN_HOME

# Compile
ninja

