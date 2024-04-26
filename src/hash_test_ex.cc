#include <iostream>

#include "hash.h"

int main() {
    std::string data = "Hallo my name is Quinten Alexander.";
    std::string digest = hashing::hash(data);
    std::cout << digest << std::endl;
    digest = hashing::hash(data);
    std::cout << digest << std::endl;
    data = "Hallo my name us Quinten Alexander.";
    digest = hashing::hash(data);
    std::cout << digest << std::endl;
    digest = hashing::hash(data);
    std::cout << digest << std::endl;
}
