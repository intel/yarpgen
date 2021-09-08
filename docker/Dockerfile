FROM ubuntu:20.04

ENV TESTING_HOME=/testing

ENV GCC_HOME=$TESTING_HOME/gcc
ARG GCC_REPO
ENV GCC_REPO=${GCC_REPO:-git://gcc.gnu.org/git/gcc.git}
ARG GCC_VERSION
ENV GCC_VERSION=${GCC_VERSION:-master}

ENV LLVM_HOME=$TESTING_HOME/llvm
ARG LLVM_REPO
ENV LLVM_REPO=${LLVM_REPO:-https://github.com/llvm/llvm-project.git}
ARG LLVM_VERSION
ENV LLVM_VERSION=${LLVM_VERSION:-main}

ARG SDE_REPO
ENV SDE_REPO=${SDE_REPO:-https://software.intel.com/content/dam/develop/external/us/en/documents/downloads/}
ARG SDE_VERSION
ENV SDE_VERSION=${SDE_VERSION:-sde-external-8.69.1-2021-07-18-lin}

ENV YARPGEN_HOME=$TESTING_HOME/yarpgen
ARG YARPGEN_REPO
ENV YARPGEN_REPO=${YARPGEN_REPO:-https://github.com/intel/yarpgen}
ARG YARPGEN_VERSION
ENV YARPGEN_VERSION=${YARPGEN_VERSION:-main}

RUN apt-get -y update &&\
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    cmake \
    curl \
    flex \
    gcc \
    gcc-multilib \
    g++ \
    git \
    ninja-build \
    python3 \
    wget
# No need to do cleanup, ubuntu container will do it for us

WORKDIR $TESTING_HOME

COPY build-gcc.sh $TESTING_HOME/
RUN ["/bin/bash", "-c", "./build-gcc.sh"]

COPY build-llvm.sh $TESTING_HOME/
RUN ["/bin/bash", "-c", "./build-llvm.sh"]

RUN wget $SDE_REPO/$SDE_VERSION.tar.bz2
RUN tar -xf $SDE_VERSION.tar.bz2
ENV PATH=$TESTING_HOME/$SDE_VERSION:$PATH

COPY build-yarpgen.sh $TESTING_HOME/
RUN ["/bin/bash", "-c", "./build-yarpgen.sh"]

ENV PATH=$GCC_HOME/bin/bin:$PATH
ENV LD_LIBRARY_PATH=$GCC_HOME/bin/lib64/:$LD_LIBRARY_PATH

ENV PATH=$LLVM_HOME/bin/bin/:$PATH
ENV LD_LIBRARY_PATH=$LLVM_HOME/bin/lib/:$LD_LIBRARY_PATH

ARG RESULT_DIR
ENV RESULT_DIR=${RESULT_DIR:-$TESTING_HOME/result}

COPY docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["-t 1"]

