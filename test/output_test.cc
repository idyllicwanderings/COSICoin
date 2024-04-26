#include "blockchain/output.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

TEST(OutputTest, Getters) {
    blockchain::Output output1(100, 1000);
    EXPECT_EQ(output1.getValue(), 100);
    EXPECT_EQ(output1.getReceiverID(), 1000);
}

// TEST(OutputTest, ProtoConvert) {
//     blockchain::Output output1(100, 1000);
//     chat::Output proto_output1 = output1.toProtoOutput();
//     blockchain::Output output2(proto_output1);
//     EXPECT_EQ(output1.getValue(), 100);
//     EXPECT_EQ(output1.getReceiverID(), 1000);
// }

TEST(OutputTest, EqualOperator) {
    blockchain::Output output1(100, 1000);
    blockchain::Output output2(100, 1000);
    EXPECT_EQ(output1, output2);
}
