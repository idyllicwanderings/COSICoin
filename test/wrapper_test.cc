#include "interf/wrapper.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

#include <iostream>

TEST(WrapperTest, CalculateHash) {
    std::string input_hash = "7854210022186565";
    wrapper::Wrapper wrapper;
    std::string output2 = wrapper.calculate_hash(input_hash);
    EXPECT_NE(input_hash, output2);
}