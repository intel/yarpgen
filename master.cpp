/*
Copyright (c) 2015-2016, Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "master.h"

unsigned int MAX_LOOP_NUM = 10;
unsigned int MIN_LOOP_NUM = 1;

Master::Master (std::string _folder) {
    this->folder = _folder;
    std::uniform_int_distribution<unsigned int> loop_num_dis(MIN_LOOP_NUM, MAX_LOOP_NUM);
    std::uniform_int_distribution<unsigned int> reuse_dis(0, 100);
    unsigned int loop_num = loop_num_dis(rand_gen);
    for (int i = 0; i < loop_num; ++i) {
        Loop tmp_loop;
        if (reuse_dis(rand_gen) < 40 && i > 0)
            tmp_loop.random_fill (i, true, loops.at(i - 1));
        else
            tmp_loop.random_fill (i, false, tmp_loop);
        loops.push_back(tmp_loop);
    }
}

void Master::write_file (std::string of_name, std::string data) {
    std::ofstream out_file;
    out_file.open (folder + "/" + of_name);
    out_file << data;
    out_file.close ();
}

std::string Master::emit_main () {
    std::string ret = "#include \"init.h\"\n\n";
    ret += "extern void init ();\n";
    ret += "extern void foo ();\n";
    ret += "extern std::size_t checksum ();\n";
    ret += "int main () {\n";
    ret += "\tinit ();\n";
    ret += "\tfoo ();\n";
    ret += "\tstd::cout << checksum () << std::endl;\n";
    ret += "\treturn 0;\n";
    ret += "}";
    write_file("driver.cpp", ret);
    return ret;
}

std::string Master::emit_init () {
    std::string ret = "#include \"init.h\"\n\n";
    for (auto i = loops.begin (); i != loops.end (); ++i)
        ret += i->emit_array_decl("", "", false);
    ret += "void init () {\n";
    for (auto i = loops.begin (); i != loops.end (); ++i) {
        ret += i->emit_array_def ();
        ret += "\n";
    }
    ret += "}";
    write_file("init.cpp", ret);
    return ret;
}

std::string Master::emit_func() {
    std::string ret = "#include \"init.h\"\n\n";
    ret += "void foo () {\n\t";
    for (auto i = loops.begin (); i != loops.end (); ++i) {
        ret += i->emit_body ();
        ret += "\n\t";
    }
    ret += "}";
    write_file("func.cpp", ret);
    return ret;
}

std::string Master::emit_check() {
    std::string ret = "#include \"init.h\"\n\n";
    ret += "std::size_t checksum () {\n\t";
    ret += "std::size_t res = 0;\n\t";
    for (auto i = loops.begin (); i != loops.end (); ++i) {
        std::string iter_name = "i_" + i->get_out_num_str ();
        ret += "for (uint64_t " + iter_name + " = 0; " + iter_name + " < " + std::to_string(i->get_min_size ()) + "; ++" + iter_name + ") {\n\t";
        ret += i->emit_array_usage("\thash(res, ", ")", true);
        ret += "}\n\n\t";
    }
    ret += "return res;\n";
    ret += "}";
    write_file("check.cpp", ret);
    return ret;
}

std::string Master::emit_hash() {
    std::string ret = "#include <boost/functional/hash.hpp>\n";
    ret += "void hash(size_t & seed, uint64_t const& v) {\n";
    ret += "\tboost::hash_combine(seed, v);\n";
    ret += "}";
    write_file("hash.cpp", ret);
    return ret;
}

std::string Master::emit_decl() {
    std::string ret = "#include <cstdint>\n";
    ret += "#include <iostream>\n";
    ret += "#include <array>\n";
    ret += "#include <vector>\n";
    ret += "void hash(size_t & seed, uint64_t const& v);\n";
    for (auto i = loops.begin (); i != loops.end (); ++i)
        ret += i->emit_array_decl("extern ", " __attribute__((aligned(32)))", true);
    write_file("init.h", ret);
    return ret;
}
