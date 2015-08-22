#include <iostream>
#include <cstdint>

extern uint64_t checksum ();

int main () {
    std::cout << checksum () << std::endl;
    return 0;
}
