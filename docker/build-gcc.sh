#!/bin/bash

if [[ -z "${TESTING_HOME}" ]] || [[ -z "${GCC_HOME}" ]] || [[ -z "${GCC_REPO}" ]] || [[ -z "${GCC_VERSION}" ]]; then
    echo "Error! There are undefined environmental variables!"
    exit 127
fi

mkdir -p $GCC_HOME
cd $GCC_HOME
# Get the source code
git clone $GCC_REPO gcc_src
mkdir build bin
cd $GCC_HOME/gcc_src
git checkout $GCC_VERSION
echo $GCC_REPO:$GCC_VERSION > $GCC_HOME/gcc_rev.txt
git log --format="%H" -n 1 >> $GCC_HOME/gcc_rev.txt

# Pre-compilation steps
contrib/download_prerequisites
# We can use the same script to build the compiler, but it uses bootstrap and it is hard to disable it
contrib/gcc_build -d $GCC_HOME/gcc_src \
    -o $GCC_HOME/build \
    -c "--enable-multilib --prefix=$GCC_HOME/bin --disable-bootstrap" \
    configure

# Compilation
cd $GCC_HOME/build
make -j 128
make -j 128 install

# Cleanup
rm -rf $GCC_HOME/build $GCC_HOME/gcc_src

