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

#ifndef NAMEGEN_H
#define NAMEGEN_H

#include <crosschain/typedefs.h>

namespace crosschain {

class ItNameGen {
public:
    static std::vector<char> pieces;
    static std::vector<ull> cnt;

/*
    static std::string getName () {
        std::stringstream ss;
        ull shortest = get_shortest_it ();
        for (ull i = 0; i < cnt.at(shortest) + 1; ++i)
            ss << pieces.at(shortest);

        cnt.at(shortest) ++;
        return ss.str();
    }
*/
    static std::string getName () {
        std::stringstream ss;
        ull shortest = get_shortest_it ();
        ss << pieces.at(shortest) << cnt.at(shortest);
        cnt.at(shortest) ++;
        return ss.str();
    }

    static ull get_shortest_it () {
        ull ret = 0;
        ull shortest = cnt.at(ret);
        for (ull i = 0; i < pieces.size(); ++i) {
            if (cnt.at(i) < shortest) {
                ret = i;
                shortest = cnt.at(i);
            }
        }
        return ret;
    }
};


class SclNameGen {
public:
    static std::vector<char> pieces;
    static std::vector<ull> cnt;

    static std::string getName () {
        std::stringstream ss;
        ull shortest = get_shortest_it ();
        ss << pieces.at(shortest) << cnt.at(shortest);
        cnt.at(shortest) ++;
        return ss.str();
    }

    static ull get_shortest_it () {
        ull ret = 0;
        ull shortest = cnt.at(ret);
        for (ull i = 0; i < pieces.size(); ++i) {
            if (cnt.at(i) < shortest) {
                ret = i;
                shortest = cnt.at(i);
            }
        }
        return ret;
    }
};


class VecNameGen {
public:
    static std::vector<char> pieces;
    static std::vector<ull> cnt;

    static std::string getName () {
        std::stringstream ss;
        ull shortest = get_shortest_it ();
        ss << pieces.at(shortest) << cnt.at(shortest);
        cnt.at(shortest) ++;
        return ss.str();
    }

    static ull get_shortest_it () {
        ull ret = 0;
        ull shortest = cnt.at(ret);
        for (ull i = 0; i < pieces.size(); ++i) {
            if (cnt.at(i) < shortest) {
                ret = i;
                shortest = cnt.at(i);
            }
        }
        return ret;
    }
};


class ScopeIdGen {
protected:
    static ull cnt;

public:
    static ull getNewID() {cnt++; return cnt;}

};
}

#endif
