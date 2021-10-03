FROM ubuntu:20.04

ENV TESTING_HOME=/testing

ENV GCC_HOME=$TESTING_HOME/gcc
ARG GCC_REPO=git://gcc.gnu.org/git/gcc.git
ARG GCC_VERSION=master

ENV LLVM_HOME=$TESTING_HOME/llvm
ARG LLVM_REPO=https://github.com/llvm/llvm-project.git
ARG LLVM_VERSION=main

ARG SDE_REPO=https://software.intel.com/content/dam/develop/external/us/en/documents/downloads/
ARG SDE_VERSION=sde-external-8.69.1-2021-07-18-lin

ENV YARPGEN_HOME=$TESTING_HOME/yarpgen
ARG YARPGEN_REPO=https://github.com/intel/yarpgen
ARG YARPGEN_VERSION=main

RUN apt-get -y update &&\
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    cmake \
    creduce \
    curl \
    flex \
    gcc \
    gcc-multilib \
    g++ \
    git \
    ninja-build \
    python3 \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR $TESTING_HOME

RUN git clone $YARPGEN_REPO $YARPGEN_HOME &&\
    cd $YARPGEN_HOME &&\
    git checkout $YARPGEN_VERSION

RUN $YARPGEN_HOME/scripts/build_gcc.sh $GCC_REPO $GCC_VERSION

RUN $YARPGEN_HOME/scripts/build_llvm.sh $LLVM_REPO $LLVM_VERSION

RUN wget $SDE_REPO/$SDE_VERSION.tar.bz2
RUN tar -xf $SDE_VERSION.tar.bz2
ENV PATH=$TESTING_HOME/$SDE_VERSION:$PATH

RUN $YARPGEN_HOME/scripts/build_yarpgen.sh

ENV PATH=$GCC_HOME/bin/bin:$PATH
ENV LD_LIBRARY_PATH=$GCC_HOME/bin/lib64/:$LD_LIBRARY_PATH

ENV PATH=$LLVM_HOME/bin/bin/:$PATH
ENV LD_LIBRARY_PATH=$LLVM_HOME/bin/lib/:$LD_LIBRARY_PATH

ARG RESULT_DIR
ENV RESULT_DIR=${RESULT_DIR:-$TESTING_HOME/result}

COPY docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["-t 1"]

