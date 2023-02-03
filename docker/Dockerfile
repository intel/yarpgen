FROM ubuntu:22.04

ENV TESTING_HOME=/testing

ENV GCC_HOME=$TESTING_HOME/gcc
ARG GCC_REPO=https://gcc.gnu.org/git/gcc.git
ARG GCC_VERSION=master

ENV LLVM_HOME=$TESTING_HOME/llvm
ARG LLVM_REPO=https://github.com/llvm/llvm-project.git
ARG LLVM_VERSION=main

ARG SDE_REPO=https://downloadmirror.intel.com/751535
ARG SDE_VERSION=sde-external-9.14.0-2022-10-25-lin

ENV YARPGEN_HOME=$TESTING_HOME/yarpgen
ARG YARPGEN_REPO=https://github.com/intel/yarpgen
ARG YARPGEN_VERSION=main

ENV BINUTILS_HOME=$TESTING_HOME/binutils
ARG BINUTILS_REPO=https://sourceware.org/git/binutils-gdb.git

ARG USER_AGENT="Mozilla/5.0"

RUN apt-get -y update &&\
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    bison \
    bzip2 \
    cmake \
    creduce \
    curl \
    flex \
    gcc \
    gcc-multilib \
    g++ \
    git \
    libgmp-dev \
    libmpfr-dev \
    libz-dev \
    ninja-build \
    python3 \
    texinfo \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR $TESTING_HOME

RUN git clone $YARPGEN_REPO $YARPGEN_HOME &&\
    cd $YARPGEN_HOME &&\
    git checkout $YARPGEN_VERSION

RUN $YARPGEN_HOME/scripts/build_gcc.sh $GCC_REPO $GCC_VERSION

# We want to test Sapphire Rapids target. The problem is that
# the system default version of binutils doesn't support it yet.
# Therefore, we have to build them by ourselves.
# We should switch back to the system default version when it is updated.
RUN $YARPGEN_HOME/scripts/build_binutils.sh $BINUTILS_REPO

RUN $YARPGEN_HOME/scripts/build_llvm.sh $LLVM_REPO $LLVM_VERSION

RUN wget -U $USER_AGENT --retry-connrefused --waitretry=5 --read-timeout=20 --timeout=15 -t 5 $SDE_REPO/$SDE_VERSION.tar.xz
RUN tar -xf $SDE_VERSION.tar.xz
ENV PATH=$TESTING_HOME/$SDE_VERSION:$PATH

RUN $YARPGEN_HOME/scripts/build_yarpgen.sh

ENV PATH=$GCC_HOME/bin_$GCC_VERSION/bin:$PATH
ENV LD_LIBRARY_PATH=$GCC_HOME/bin_$GCC_VERSION/lib64/:$LD_LIBRARY_PATH

ENV PATH=$BINUTILS_HOME/bin/bin:$PATH
ENV LD_LIBRARY_PATH=$BINUTILS_HOME/bin/lib/:$LD_LIBRARY_PATH

ENV PATH=$LLVM_HOME/bin_$LLVM_VERSION/bin/:$PATH
ENV LD_LIBRARY_PATH=$LLVM_HOME/bin_$LLVM_VERSION/lib/:$LD_LIBRARY_PATH

ARG RESULT_DIR
ENV RESULT_DIR=${RESULT_DIR:-$TESTING_HOME/result}

COPY docker-entrypoint.sh /
# It is recommended to use a no-cache build, otherwise compiler won't be updated:
# docker build --no-cache -t yarpgen-testing .
#
# Use this command to start a testing for 24-hours.
# Feel free to change the parameters that are passed to run_gen.py
# docker run --rm --name yarpgen-daily --mount source=yarpgen-result,target=/testing/result -e HOST_HOSTNAME=$(uname -n) yarpgen-testing -t 1440 
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["-t 1"]

