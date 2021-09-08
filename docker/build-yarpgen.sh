#!/bin/bash

if [[ -z "${TESTING_HOME}" ]] || [[ -z "${YARPGEN_HOME}" ]] || [[ -z "${YARPGEN_REPO}" ]] || [[ -z "${YARPGEN_VERSION}" ]]; then
    echo "Error! There are undefined environmental variables!"
    exit 127
fi

# Get the source code
mkdir -p $YARPGEN_HOME
cd $YARPGEN_HOME
git clone $YARPGEN_REPO $YARPGEN_HOME
mkdir build
git checkout $YARPGEN_VERSION
echo $YARPGEN_REPO:$YARPGEN_VERSION > $YARPGEN_HOME/yarpgen_rev.txt
git log --format="%H" -n 1 >> $YARPGEN_HOME/yarpgen_rev.txt

# Pre-compilation steps
cd $YARPGEN_HOME/build
cmake -G "Ninja" $YARPGEN_HOME

# Compile
ninja

