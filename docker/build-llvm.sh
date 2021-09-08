#!/bin/bash

if [[ -z "${TESTING_HOME}" ]] || [[ -z "${LLVM_HOME}" ]] || [[ -z "${LLVM_REPO}" ]] || [[ -z "${LLVM_VERSION}" ]]; then
    echo "Error! There are undefined environmental variables!"
    exit 127
fi

# Get the source code
mkdir -p $LLVM_HOME
cd $LLVM_HOME
git clone $LLVM_REPO llvm_src
mkdir build bin
cd $LLVM_HOME/llvm_src
git checkout $LLVM_VERSION
echo $LLVM_REPO:$LLVM_VERSION > $LLVM_HOME/llvm_rev.txt
git log --format="%H" -n 1 >> $LLVM_HOME/llvm_rev.txt

# Pre-compilation steps
cd $LLVM_HOME/build
cmake -G "Ninja" \
    -DCMAKE_INSTALL_PREFIX=$LLVM_HOME/bin \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_INSTALL_UTILS=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_ENABLE_PROJECTS="clang;compiler-rt;clang-tools-extra;polly" \
    $LLVM_HOME/llvm_src/llvm &&\

# Compile
ninja install

# Cleanup
rm -rf $LLVM_HOME/build $LLVM_HOME/llvm_src

