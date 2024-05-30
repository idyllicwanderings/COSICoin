#include "interf/wrapper.h"

using namespace wrapper;

std::string Wrapper::calculate_hash(std::string input_hash) {
    uint32_t size = static_cast<uint32_t>(input_hash.size() * 8);
    uint32_t number_blocks;
    if (size < 448) number_blocks = 0x0;
    else number_blocks = 0x1;
    std::string padded_string = padding(input_hash);    
    const char *c_string = padded_string.c_str();
    char outputString[(ARRAY_SIZE * 8) + 1]; //output of the hash results in 256 bits
    double* elapsed_time;
    calculate_hash_c(c_string, outputString, number_blocks, elapsed_time);
    std::string cpp_string_return = std::string(outputString);
    return cpp_string_return;
}

double* Wrapper::time_hw_hash(std::string input_hash) {
    uint32_t size = static_cast<uint32_t>(input_hash.size() * 8);
    uint32_t number_blocks;
    if (size < 448) number_blocks = 0x0;
    else number_blocks = 0x1;
    std::string padded_string = padding(input_hash);    
    const char *c_string = padded_string.c_str();
    char outputString[(ARRAY_SIZE * 8) + 1]; //output of the hash results in 256 bits
    double* elapsed_time;
    calculate_hash_c(c_string, outputString, number_blocks, elapsed_time);
    std::string cpp_string_return = std::string(outputString);
    return elapsed_time;
}



std::string Wrapper::padding(std::string input_string) {
    std::string bin_string = charToBinaryString(input_string);
    int len = bin_string.length();
    std::string str_len = charToBinaryString(std::to_string(len));
    int len_str_len = str_len.length();
    if (len < 448) {
        bin_string += "1";
        for (int i = 0; i < 512 - len - len_str_len - 1; i++) {
            bin_string += "0";
        }
        bin_string += str_len;
    } else {
        bin_string += "1";
        for (int i = 0; i < 512+512 - len - len_str_len - 1; i++) {
            bin_string += "0";
        }
        bin_string += str_len;
    }
    return bin_string;
}


std::string Wrapper::charToBinaryString(const std::string& input) {
    std::string result;
    for (char c : input) {
        // Convert each character to its ASCII value and then to binary representation
        result += std::bitset<8>(c).to_string();
    }
    return result;
}