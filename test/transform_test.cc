#include <iostream>
#include <string>
#include <cstddef>
#include <gtest/gtest.h>

TEST(transf, transform) {

    std::string cpp_string= "849845648";
    const char *c_string = cpp_string.c_str();
    std::string cpp_string_return = std::string(c_string);
    std::cerr << "cpp_string " << cpp_string << std::endl;
    std::cerr << "c_string " << c_string << std::endl;
    std::cerr << "cpp_string_return " << cpp_string_return << std::endl;
    EXPECT_EQ(cpp_string, cpp_string_return);
}