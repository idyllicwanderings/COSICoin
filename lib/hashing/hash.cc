#include "hash.h"

std::string hashing::hash(std::string data) {
    // create a new hashing object
    SHA256 sha256;
    // Calculate hash
    return sha256(data);
}