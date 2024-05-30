#include <cassert>
#include <random>
#include <string>
#include <thread>

#include "interf/wrapper.h"

#define KEY_LEN_ 256

std::string hash(const std::string input) {
    wrapper::Wrapper wrapper;
    return wrapper.calculate_hash(input);
}

std::string generate_random_string(int length, std::string check_string = "") {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    static const int charsetSize = sizeof(charset) - 1;

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, charsetSize - 1);

    std::string randomString;
    randomString.reserve(length);

    for (int i = 0; i < length; ++i) {
        randomString += charset[distribution(generator)];
    }

    while (randomString == check_string) {
        randomString = "";
        for (int i = 0; i < length; ++i) {
            randomString += charset[distribution(generator)];
        }
    }

    return randomString;
}

int main() {
    // Short input
    std::string inputA = "hello";
    std::string inputB = "bye";
    std::string result1 = hash(inputA);
    std::string result2 = hash(inputA);
    std::string result3 = hash(inputB);
    assert(result1 == result2);
    assert(result1 != result3);
    std::cout << "Short input test passed" << std::endl;

    // 256 bit input
    inputA = generate_random_string(32);
    inputB = generate_random_string(32, inputA);
    result1 = hash(inputA);
    result2 = hash(inputA);
    result3 = hash(inputB);
    assert(result1 == result2);
    assert(result1 != result3);
    std::cout << "256 bit input test passed" << std::endl;

    // 512 bit input
    inputA = generate_random_string(64);
    inputB = generate_random_string(64, inputA);
    result1 = hash(inputA);
    result2 = hash(inputA);
    result3 = hash(inputB);
    assert(result1 == result2);
    assert(result1 != result3);
    std::cout << "512 bit input test passed" << std::endl;

    // 1024 bit input
    inputA = generate_random_string(128);
    inputB = generate_random_string(128, inputA);
    result1 = hash(inputA);
    result2 = hash(inputA);
    result3 = hash(inputB);
    assert(result1 == result2);
    assert(result1 != result3);
    std::cout << "1024 bit input test passed" << std::endl;

    // Wait on input from user
    std::string input;
    while (input != "q") {
        std::cout << "Enter a string to hash (q to quit): ";
        std::cin >> input;
        if (input != "q") {
            std::string h = hash(input);
            std::cout << "Hash: " << h << " size: " << h.size() << std::endl;
            input = "";
        }
    }
}
