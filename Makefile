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

CXX=clang++
CXXFLAGS=-std=c++11 -I./ -Icrosschain/include
OPT=-O3
LDFLAGS=-L./ -Lcrosschain/lib
LIBSOURCES=variable.cpp node.cpp type.cpp gen_policy.cpp generator.cpp sym_table.cpp new-master.cpp
SOURCES=main.cpp $(LIBSOURCES)
LIBOBJS=$(addprefix objs/, $(LIBSOURCES:.cpp=.o))
OBJS=$(addprefix objs/, $(SOURCES:.cpp=.o))
HEADERS=node.h type.h variable.h gen_policy.h generator.h sym_table.h new-master.h
EXECUTABLE=yarpgen

CC_CFLAGS=-c -I./ -Icrosschain/include -std=c++11 -g -Werror
CC_LDFLAGS=-std=c++11 -L./lib -L../yarpgen
CC_HEADERS := $(shell find crosschain/include -name '*.h')
CC_SOURCEFILES=types.cpp expr.cpp stmt.cpp interface.cpp namegen.cpp lbbuilder.cpp
CC_SOURCES=$(addprefix crosschain/src/,$(CC_SOURCEFILES))
CC_OBJECTS=$(addprefix crosschain/objs/,$(CC_SOURCEFILES:.cpp=.o))

default: $(EXECUTABLE)

$(EXECUTABLE): dir $(SOURCES) $(HEADERS) $(OBJS) libyarpgen libcrosschain
	$(CXX) $(OPT) $(LDFLAGS) -o $@ $(OBJS) -lcrosschain

libyarpgen: dir $(LIBSOURCES) $(HEADERS) $(LIBOBJS)
	ar rcs $@.a $(LIBOBJS)

libcrosschain: $(CC_HEADERS) $(CC_OBJECTS) dir
	ar rcs crosschain/lib/$@.a $(CC_OBJECTS)

crosschain/objs/%.o: crosschain/src/%.cpp dir
	$(CXX) $(CC_CFLAGS) $< -o $@


dir:
	/bin/mkdir -p objs crosschain/objs crosschain/lib

clean:
	/bin/rm -rf objs $(EXECUTABLE) libyarpgen.a crosschain/lib crosschain/objs

debug: $(EXECUTABLE)
debug: OPT=-O0 -g

ubsan: $(EXECUTABLE)
ubsan: CXXFLAGS+=-fsanitize=undefined

gcc: $(EXECUTABLE)
gcc: CXX=g++

gcov: $(EXECUTABLE) 
gcov: CXX=g++
gcov: OPT+=-fprofile-arcs -ftest-coverage -g

objs/%.o: %.cpp $(HEADERS)
	$(CXX) $(OPT) $(CXXFLAGS) -o $@ -c $<
