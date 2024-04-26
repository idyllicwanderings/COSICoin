#ifndef COSICOIN_HASH_H
#define COSICOIN_HASH_H

#include <sha256.h>

#include <string>

namespace hashing {

std::string hash(std::string data);

}  // namespace hashing

#endif