#!/bin/bash -e
# Args: GCC_REPO GCC_VERSION

if [[ -z "${GCC_HOME}" ]]; then
    echo "Error! Environmental variable for GCC_HOME is not set!"
    exit 127
fi

GCC_REPO=$1
GCC_VERSION=$2

if [[ -z "${GCC_REPO}" ]] || [[ -z "${GCC_VERSION}" ]]; then
    echo "Error! Parameters for GCC repository are incorrect"
    exit 127
fi

mkdir -p $GCC_HOME
cd $GCC_HOME
# Get the source code
git clone $GCC_REPO gcc_src_$GCC_VERSION
mkdir build_$GCC_VERSION bin_$GCC_VERSION
cd $GCC_HOME/gcc_src_$GCC_VERSION
git checkout $GCC_VERSION
echo $GCC_REPO:$GCC_VERSION > $GCC_HOME/gcc_rev.txt
git log --format="%H" -n 1 >> $GCC_HOME/gcc_rev.txt

# Pre-compilation steps
contrib/download_prerequisites
# We can use the same script to build the compiler, but it uses bootstrap and it is hard to disable it
contrib/gcc_build -d $GCC_HOME/gcc_src_$GCC_VERSION \
    -o $GCC_HOME/build_$GCC_VERSION \
    -c "--enable-multilib --prefix=$GCC_HOME/bin_$GCC_VERSION --disable-bootstrap" \
    configure

# Compilation
cd $GCC_HOME/build_$GCC_VERSION
make -j `nrpoc`
make -j `nproc` install

# Cleanup
rm -rf $GCC_HOME/build_$GCC_VERSION $GCC_HOME/gcc_src_$GCC_VERSION

