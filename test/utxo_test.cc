#include "blockchain/utxo.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

#include "blockchain/input.h"
#include "blockchain/output.h"

TEST(UTXOTest, Getters) {
    // UTXO
    blockchain::Output output(120, 2);
    blockchain::UTXO utxo(11, 5, output);
    EXPECT_EQ(output, utxo.getOutput());
    EXPECT_EQ(11, utxo.getTransactionId());
    EXPECT_EQ(5, utxo.getOutputIndex());

    // UTXOlist
    blockchain::Output output2(32, 3);
    blockchain::UTXO utxo2(16, 4, output2);
    blockchain::Output output3(101, 19);
    blockchain::UTXO utxo3(12, 8, output3);

    std::vector<blockchain::UTXO> utxol{utxo, utxo2, utxo3};
    blockchain::UTXOlist utxolist(utxol);
    EXPECT_EQ(3, utxolist.size());
    EXPECT_EQ(utxo, utxolist[0]);
    EXPECT_EQ(utxo2, utxolist[1]);
    EXPECT_EQ(utxo3, utxolist[2]);
    utxolist.clear();
    EXPECT_EQ(0, utxolist.size());
    EXPECT_TRUE(utxolist.empty());
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

TEST(UTXOTest, UTXOlistGetters) {
    blockchain::UTXOlist utxolist;
    EXPECT_TRUE(utxolist.empty());
    EXPECT_EQ(0, utxolist.size());
    blockchain::Output output1(120, 2);
    blockchain::UTXO utxo1(11, 5, output1);
    blockchain::Output output2(32, 3);
    blockchain::UTXO utxo2(16, 4, output2);
    blockchain::Output output3(101, 19);
    blockchain::UTXO utxo3(12, 8, output3);
    utxolist.add(utxo1);
    utxolist.add(utxo2);
    utxolist.add(utxo3);
    EXPECT_EQ(3, utxolist.size());
    EXPECT_EQ(utxo1, utxolist[0]);
    EXPECT_EQ(utxo2, utxolist[1]);
    EXPECT_EQ(utxo3, utxolist[2]);
    blockchain::Input input1(11, 5);
    EXPECT_EQ(0, utxolist.index(input1));
    blockchain::Input input2(16, 4);
    EXPECT_EQ(1, utxolist.index(input2));
    blockchain::Input input3(12, 8);
    EXPECT_EQ(2, utxolist.index(input3));
    utxolist.erase(1);
    EXPECT_EQ(2, utxolist.size());
    EXPECT_EQ(utxo1, utxolist[0]);
    EXPECT_EQ(utxo3, utxolist[1]);
    EXPECT_EQ(-1, utxolist.index(input2));
    utxolist.clear();
    EXPECT_TRUE(utxolist.empty());
}

TEST(UTXOTest, UTXOlistProtoConvert) {
    blockchain::Output output1(120, 2);
    blockchain::UTXO utxo1(11, 5, output1);
    blockchain::Output output2(32, 3);
    blockchain::UTXO utxo2(16, 4, output2);
    blockchain::Output output3(101, 19);
    blockchain::UTXO utxo3(12, 8, output3);

    std::vector<blockchain::UTXO> utxol{utxo1, utxo2, utxo3};
    blockchain::UTXOlist utxolist(utxol);
    EXPECT_EQ(utxol, utxolist.getUTXOList());

    // Convert
    chat::UTXOlist proto_list;
    utxolist.toProtoUTXOlist(&proto_list);
    blockchain::UTXOlist rec_utxolist1(proto_list);
    blockchain::UTXOlist rec_utxolist2;
    rec_utxolist2.fromProtoUTXOlist(proto_list);

    EXPECT_EQ(utxol, rec_utxolist1.getUTXOList());
    EXPECT_EQ(utxol, rec_utxolist2.getUTXOList());
}
