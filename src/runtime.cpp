#include <iostream>

#include "mila.h"
#include "runtime.h"

extern "C" int read_() {
    int result;
    std::cout << "Zadejte cislo: " ;
    std::cin >> result;
    return result;
}

extern "C" void write_(int what) {
    std::cout << "Vypis: " << what << std::endl;
}
