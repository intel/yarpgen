#!/bin/bash -e
# Args: LLVM_REPO LLVM_VERSION

if [[ -z "${LLVM_HOME}" ]]; then
    echo "Error! Environmental variable for LLVM_HOME is not set!"
    exit 127
fi

LLVM_REPO=$1
LLVM_VERSION=$2

if [[ -z "${LLVM_REPO}" ]] || [[ -z "${LLVM_VERSION}" ]]; then
    echo "Error! Parameters for LLVM repository are incorrect"
    exit 127
fi

# Get the source code
mkdir -p $LLVM_HOME
cd $LLVM_HOME
git clone $LLVM_REPO llvm_src_$LLVM_VERSION
mkdir build_$LLVM_VERSION bin_$LLVM_VERSION
cd $LLVM_HOME/llvm_src_$LLVM_VERSION
git checkout $LLVM_VERSION
echo $LLVM_REPO:$LLVM_VERSION > $LLVM_HOME/llvm_rev.txt
git log --format="%H" -n 1 >> $LLVM_HOME/llvm_rev.txt

# Pre-compilation steps
cd $LLVM_HOME/build_$LLVM_VERSION
cmake -G "Ninja" \
    -DCMAKE_INSTALL_PREFIX=$LLVM_HOME/bin_$LLVM_VERSION \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_INSTALL_UTILS=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_ENABLE_PROJECTS="clang;compiler-rt;clang-tools-extra;polly" \
    $LLVM_HOME/llvm_src_$LLVM_VERSION/llvm &&\

# Compile
ninja install

# Cleanup
rm -rf $LLVM_HOME/build_$LLVM_VERSION $LLVM_HOME/llvm_src_$LLVM_VERSION

