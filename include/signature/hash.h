#ifndef COSICOIN_HASH_H
#define COSICOIN_HASH_H

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <bitset>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "json/json.hpp"
#include "sha256.h"

#define KEY_LEN_ 256

using json = nlohmann::json;

namespace signature {

std::string hash(std::string data);

/*
 * Converts the bits to a string. The bits are grouped in bytes of 8 bits and every byte is converted to a char in the string
 */
std::string bitset_to_string(std::bitset<KEY_LEN_> bits);

/*
 * Converts the string to bits. Every character is read as a byte and the bits in the byte are added to the bitset
 */
std::bitset<KEY_LEN_> string_to_bitset(std::string str);

std::string signature_to_json(std::vector<std::string> signature);
std::vector<std::string> json_to_string(std::string string);

struct SigKey {
    std::vector<std::string> S0;
    std::vector<std::string> S1;

    friend bool operator==(const SigKey& key1, const SigKey& key2) {
        return key1.S0 == key2.S0 && key1.S1 == key2.S1;
    };

    friend bool operator!=(const SigKey& key1, const SigKey& key2) {
        return key1.S0 != key2.S0 || key1.S1 != key2.S1;
    };
};

std::string SigKey_to_string(SigKey sigkey);
SigKey SigKey_from_string(std::string sigkey_string);
std::string print_SigKey(SigKey sigkey);

/*
 * @brief verify the siganture
 *
 * @param message m, the corresponding signature sig
 * @return -1 invalid key or sig
 * @return 0 verify fail
 * @return 1 verify success
 */
int Verify(std::string m, std::vector<std::string> sig, SigKey public_key);

/*
 * Signs the message, returns the signature
 * Returns empty vector if key is not generated
 */
std::vector<std::string> Sign(std::string m, SigKey private_key);

class Signature {
   public:
    // Singleton
    static Signature& getInstance() {
        static Signature s;
        return s;
    }

    /*
     * @brief key generation
     *
     * @param
     * @return 0 generation fail
     * @return 1 generation success
     */
    int KeyGen();

    SigKey getPublicKey() const { return public_key_; }
    SigKey getPrivateKey() const { return private_key_; }

   private:
    Signature() { ; }

    /*
     * Generates random bitset of size KEY_LEN_
     */
    std::bitset<KEY_LEN_> getRandomBits();

    SigKey public_key_;
    SigKey private_key_;
};

}  // namespace signature

#endif