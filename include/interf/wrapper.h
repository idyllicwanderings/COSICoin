#ifndef CHAT_WRAPPER_H
#define CHAT_WRAPPER_H

#define ARRAY_SIZE 32
#define FPGA_BASE_ADDR 0x40400000
#define ARRAY_SIZE2 8


#ifdef __cplusplus
// includes for cpp
#include <string>
#include <cstddef>
#include <iostream>
#include <bitset>
#endif

// includes for c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>  
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif
void calculate_hash_c(const char* input, char* output, uint32_t number_blocks, double* elapsed_time);
void interface_c(const uint32_t* block, uint32_t number_blocks, uint32_t* output);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace wrapper {

class Wrapper {
    public:
    Wrapper() = default;

    std::string calculate_hash(std::string input_hash);

    double* time_hw_hash(std::string input);   

    std::string padding(std::string input_string);

    std::string charToBinaryString(const std::string& input);


};
}
#endif

#endif