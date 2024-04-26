#include "blockchain/input.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

TEST(InputTest, Getters) {
    blockchain::Input input1(1000, 1);
    EXPECT_EQ(input1.getOutputIndex(), 1);
    EXPECT_EQ(input1.getTxID(), 1000);
}

// TEST(InputTest, ProtoConvert) {
//     blockchain::Input input1(1000, 1);
//     chat::Input proto_input1 = input1.toProtoInput();
//     blockchain::Input input2(proto_input1);
//     EXPECT_EQ(input2.getOutputIndex(), 1);
//     EXPECT_EQ(input2.getTxID(), 1000);
// }

TEST(InputTest, EqualOperator) {
    blockchain::Input input1(1000, 1);
    blockchain::Input input2(1000, 1);
    EXPECT_EQ(input1, input2);
}
