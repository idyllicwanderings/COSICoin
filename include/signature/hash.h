#ifndef COSICOIN_HASH_H
#define COSICOIN_HASH_H

#include "sha256.h"
#include <string>
#include <iostream>

#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <limits.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define KEY_LEN_ 256

namespace signature {

std::string hash(std::string data);

// @overload, protobuf does not support template features
unsigned char* hash(unsigned char* data);

unsigned char hash(unsigned char data);

struct SigKey {
    unsigned char S0[KEY_LEN_];
    unsigned char S1[KEY_LEN_];

    friend bool operator==(const SigKey& key1, const SigKey& key2) {
        for (int i = 0; i < KEY_LEN_; i++) {
            if (key1.S0[i] != key2.S0[i] || key1.S1[i] != key2.S1[i]) {
                return false;
            }
        }
        return true;
    };
};

/*
* @brief verify the siganture
*
* @param message m, the corresponding signature sig
* @return 0 verify fail
* @return 1 verify success
*/
int Verify(unsigned char m, unsigned char* sig, SigKey public_key);


inline void Char2Binary(unsigned char m, int bin[]) {
    for (int i = 0; i < KEY_LEN_; i++) {
        bin[i] = (m >> i) & 1;
    }
}

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

        /*
        * @brief sign the input message
        *
        * @param message m, output signature sig
        * @return 0 sign fail
        * @return 1 sign success
        */
        int Sign(unsigned char m, unsigned char* sig);



        SigKey getPublicKey() const {return public_key_;}

    private:
        Signature() {;}
      
        /*
        * @brief returns a random byte of length 
        *
        * @param bytestream, length, using /dev/random or /dev/urandom
        * @return 0 generation fail
        * @return 1 generation success
        */
        int getRandomBytes(unsigned char *buf, int length, int block);

        

        SigKey public_key_;
        SigKey private_key_;

};

}  // namespace signature

#endif