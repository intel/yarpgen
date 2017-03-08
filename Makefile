###############################################################################
#
# Copyright (c) 2015-2016, Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################

BUILD_DATE=$(shell date +%Y:%m:%d)
GIT_REV=$(shell git log --abbrev-commit --abbrev=16 2>/dev/null | head -1)
ifeq (${GIT_REV},)
	BUILD_VERSION="no_version_info"
else
	BUILD_VERSION=$(GIT_REV)
endif

CXX=clang++
CXXFLAGS=-std=c++14 -Wall -Wpedantic -Werror -DBUILD_DATE="\"$(BUILD_DATE)\"" -DBUILD_VERSION="\"$(BUILD_VERSION)\""
OPT=-O3
LDFLAGS=-L./ -std=c++14
LIBSOURCES=type.cpp variable.cpp expr.cpp stmt.cpp gen_policy.cpp sym_table.cpp master.cpp
SOURCES=main.cpp $(LIBSOURCES) self-test.cpp
LIBSOURCES_SRC=$(addprefix src/, $(LIBSOURCES))
SOURCES_SRC=$(addprefix src/, $(SOURCES))
LIBOBJS=$(addprefix objs/, $(LIBSOURCES:.cpp=.o))
OBJS=$(addprefix objs/, $(SOURCES:.cpp=.o))
HEADERS=type.h variable.h ir_node.h expr.h stmt.h gen_policy.h sym_table.h master.h
HEADERS_SRC=$(addprefix src/, $(HEADERS))
EXECUTABLE=yarpgen

default: $(EXECUTABLE)

$(EXECUTABLE): dir $(SOURCES_SRC) $(HEADERS_SRC) $(OBJS) libyarpgen
	$(CXX) $(OPT) $(LDFLAGS) -o $@ $(OBJS)

libyarpgen: dir $(LIBSOURCES_SRC) $(HEADERS_SRC) $(LIBOBJS)
	ar rcs $@.a $(LIBOBJS)

dir:
	/bin/mkdir -p objs

clean:
	/bin/rm -rf objs $(EXECUTABLE) libyarpgen.a

debug: $(EXECUTABLE)
debug: OPT=-O0 -g

ubsan: $(EXECUTABLE)
ubsan: CXXFLAGS+=-fsanitize=undefined

gcc: $(EXECUTABLE)
gcc: CXX=g++

gcov: $(EXECUTABLE) 
gcov: CXX=g++
gcov: OPT+=-fprofile-arcs -ftest-coverage -g

objs/%.o: src/%.cpp $(HEADERS_SRC)
	$(CXX) $(OPT) $(CXXFLAGS) -o $@ -c $<
