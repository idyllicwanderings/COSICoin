#include "blockchain/utxo.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

#include "blockchain/output.h"

TEST(UTXOTest, Getters) {
    blockchain::Output output(120, 2);
    blockchain::UTXO utxo(11, 5, output);
    EXPECT_EQ(output, utxo.getOutput());
    EXPECT_EQ(11, utxo.getTransactionId());
    EXPECT_EQ(5, utxo.getOutputIndex());
}

TEST(UTXOTest, ProtoConvert) {
    blockchain::Output output(120, 2);
    blockchain::UTXO utxo1(11, 5, output);
    chat::UTXO proto_utxo;
    utxo1.toProtoUTXO(&proto_utxo);
    blockchain::UTXO utxo2(proto_utxo);
    EXPECT_EQ(output, utxo2.getOutput());
    EXPECT_EQ(11, utxo2.getTransactionId());
    EXPECT_EQ(5, utxo2.getOutputIndex());
    EXPECT_EQ(utxo1, utxo2);
}

TEST(UTXOTest, ProtoUTXOlist) {
    blockchain::Output output1(120, 2);
    blockchain::UTXO utxo1(11, 5, output1);
    blockchain::Output output2(32, 3);
    blockchain::UTXO utxo2(16, 4, output2);
    blockchain::Output output3(101, 19);
    blockchain::UTXO utxo3(12, 8, output3);

    // Convert to
    chat::UTXOlist utxolist;
    utxo1.toProtoUTXO(utxolist.add_utxo());
    utxo2.toProtoUTXO(utxolist.add_utxo());
    utxo3.toProtoUTXO(utxolist.add_utxo());

    // Convert from
    blockchain::UTXO rec_utxo1(utxolist.utxo(0));
    blockchain::UTXO rec_utxo2(utxolist.utxo(1));
    blockchain::UTXO rec_utxo3(utxolist.utxo(2));

    EXPECT_EQ(utxo1, rec_utxo1);
    EXPECT_EQ(utxo2, rec_utxo2);
    EXPECT_EQ(utxo3, rec_utxo3);
}
