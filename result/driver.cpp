#include <iostream>
#include <cstdint>

extern uint64_t foo ();

int main () {
    std::cout << foo () << std::endl;
    return 0;
}
