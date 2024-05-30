#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "signature/hash.h"
#include "interf/wrapper.h"

using namespace signature;

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

std::string software_hash(std::string input) {
    return signature::hash(input);
}

double time_software_hash(std::string input) {
    auto start = std::chrono::high_resolution_clock::now();
    std::string hash = software_hash(input);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

std::string hardware_hash(std::string input) {
    wrapper::Wrapper wrapper;
    return wrapper.calculate_hash(input);
}

double time_hardware_hash(std::string input) {
    auto start = std::chrono::high_resolution_clock::now();
    std::string hash = hardware_hash(input);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

double time_hw(std::string input) {
    wrapper::Wrapper wrapper;
    return *wrapper.time_hw_hash(input);
}

int main() {
    std::vector<int> sizes{1, 8, 16, 32, 64, 128};
    for (int size : sizes) {
        double software_time = 0;
        double hardware_time = 0;
        double hw_time = 0;
        int bitsize = size * 8;
        std::cout << "Timing hash for string of " << bitsize << " bits." << std::endl;
        for (int i = 0; i < 100; i++) {
            std::string input = generate_random_string(size);
            software_time += time_software_hash(input);
            hardware_time += time_hardware_hash(input);
            hw_time += time_hw(input);
        }
        std::cout << "Software mean hash time: " << software_time / 100 << " seconds." << std::endl;
        std::cout << "Hardware mean hash time: " << hardware_time / 100 << " seconds." << std::endl;
        std::cout << "Hw mean hash time: " << hw_time / 100 << " seconds." << std::endl
                  << std::endl;
    }
}