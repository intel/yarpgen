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
CXXFLAGS=-std=c++11 -Wall -Wpedantic -Werror
OPT=-O3
LDFLAGS=-L./
LIBSOURCES=type.cpp
SOURCES=main.cpp $(LIBSOURCES) self-test.cpp
LIBOBJS=$(addprefix objs/, $(LIBSOURCES:.cpp=.o))
OBJS=$(addprefix objs/, $(SOURCES:.cpp=.o))
HEADERS=type.h
EXECUTABLE=yarpgen

default: $(EXECUTABLE)

$(EXECUTABLE): dir $(SOURCES) $(HEADERS) $(OBJS) libyarpgen
	$(CXX) $(OPT) $(LDFLAGS) -o $@ $(OBJS)

libyarpgen: dir $(LIBSOURCES) $(HEADERS) $(LIBOBJS)
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

objs/%.o: %.cpp $(HEADERS)
	$(CXX) $(OPT) $(CXXFLAGS) -o $@ -c $<
