#!/bin/bash -e
# Args: BINUTILS_REPO

if [[ -z "${BINUTILS_HOME}" ]]; then
    echo "Error! Environmental variable for BINUTILS_HOME is not set!"
    exit 127
fi

BINUTILS_REPO=$1

if [[ -z "${BINUTILS_REPO}" ]]; then
    echo "Error! Parameters for BINUTILS repository are incorrect"
    exit 127
fi

mkdir -p $BINUTILS_HOME
cd $BINUTILS_HOME
# Get the source code
git clone $BINUTILS_REPO binutils_src
mkdir build bin
cd $BINUTILS_HOME/binutils_src
echo $BINUTILS_REPO > $BINUTILS_HOME/binutils_rev.txt
git log --format="%H" -n 1 >> $BINUTILS_HOME/binutils_rev.txt

# Pre-compilation steps
cd $BINUTILS_HOME/build
$BINUTILS_HOME/binutils_src/configure --prefix=$BINUTILS_HOME/bin

# Compilation
make -j `nrpoc`
make -j `nproc` install

# Cleanup
rm -rf $BINUTILS_HOME/build $BINUTILS_HOME/binutils_src

