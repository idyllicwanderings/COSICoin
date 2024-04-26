#include "blockchain/header.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

TEST(HeaderTest, Getters) {
    blockchain::Header header1("prevheader");
    EXPECT_EQ(header1.getPrevBlockDigest(), "prevheader");
}

TEST(HeaderTest, ProtoConvert) {
    blockchain::Header header1("prevheader");
    chat::Header proto_header1 = header1.toProtoHeader();
    blockchain::Header header2(proto_header1);
    EXPECT_EQ(header1.getPrevBlockDigest(), "prevheader");
}

TEST(HeaderTest, MerkleRoot) {
    blockchain::Header header("prevheader");

    // Check empty transactions list
    std::vector<blockchain::Transaction> empty_list;
    header.calculateMerkleRoot(empty_list);
    EXPECT_EQ("", header.getMerkleRoot());

    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, 1001);
    blockchain::Output output2(20, 1002);
    blockchain::Output output3(10, 1002);

    // Create transactions
    blockchain::Transaction transaction1(3001);  // 12-12
    blockchain::Transaction transaction2(3002);  // 13-13
    blockchain::Transaction transaction3(3003);  // 23-23
    blockchain::Transaction transaction4(3004);  // 12-13
    blockchain::Transaction transaction5(3005);  // 23-13
    blockchain::Transaction transaction6(3006);  // 13-23
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addInput(input1);
    transaction2.addInput(input3);
    transaction2.addOutput(output1);
    transaction2.addOutput(output3);
    transaction3.addInput(input2);
    transaction3.addInput(input3);
    transaction3.addOutput(output2);
    transaction3.addOutput(output3);
    transaction4.addInput(input1);
    transaction4.addInput(input2);
    transaction4.addOutput(output1);
    transaction4.addOutput(output3);
    transaction5.addInput(input2);
    transaction5.addInput(input3);
    transaction5.addOutput(output1);
    transaction5.addOutput(output3);
    transaction6.addInput(input1);
    transaction6.addInput(input3);
    transaction6.addOutput(output2);
    transaction6.addOutput(output3);
    std::vector<blockchain::Transaction> transactions{transaction1, transaction2, transaction3, transaction4, transaction5, transaction6};

    // Calculate transaction hashes
    std::string txs1_hash = transaction1.getDigest();
    std::string txs2_hash = transaction2.getDigest();
    std::string txs3_hash = transaction3.getDigest();
    std::string txs4_hash = transaction4.getDigest();
    std::string txs5_hash = transaction5.getDigest();
    std::string txs6_hash = transaction6.getDigest();

    // Calculate first level hashes
    std::string hash_11 = signature::hash(txs1_hash + txs2_hash);
    std::string hash_12 = signature::hash(txs3_hash + txs4_hash);
    std::string hash_13 = signature::hash(txs5_hash + txs6_hash);

    // Calculate second level hashes
    std::string hash_21 = signature::hash(hash_11 + hash_12);
    std::string hash_22 = signature::hash(hash_13 + hash_13);

    // Calculate root hash
    std::string root_hash = signature::hash(hash_21 + hash_22);

    // Calculate merkle root in header
    header.calculateMerkleRoot(transactions);
    std::string merkle_root = header.getMerkleRoot();

    // Check merkle root
    EXPECT_EQ(root_hash, merkle_root);

    // Change a transaction
    transactions[0].addInput(input3);

    // Recalculate merkle root
    header.calculateMerkleRoot(transactions);
    merkle_root = header.getMerkleRoot();
    // Check merkle root
    EXPECT_NE(root_hash, merkle_root);
}

TEST(HeaderTest, calculateDigest) {
    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, 1001);
    blockchain::Output output2(20, 1002);
    blockchain::Output output3(10, 1002);

    // Create transactions
    blockchain::Transaction transaction1(3001);  // 12-12
    blockchain::Transaction transaction2(3002);  // 13-13
    blockchain::Transaction transaction3(3003);  // 23-23
    blockchain::Transaction transaction4(3004);  // 12-13
    blockchain::Transaction transaction5(3005);  // 23-13
    blockchain::Transaction transaction6(3006);  // 13-23
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addInput(input1);
    transaction2.addInput(input3);
    transaction2.addOutput(output1);
    transaction2.addOutput(output3);
    transaction3.addInput(input2);
    transaction3.addInput(input3);
    transaction3.addOutput(output2);
    transaction3.addOutput(output3);
    transaction4.addInput(input1);
    transaction4.addInput(input2);
    transaction4.addOutput(output1);
    transaction4.addOutput(output3);
    transaction5.addInput(input2);
    transaction5.addInput(input3);
    transaction5.addOutput(output1);
    transaction5.addOutput(output3);
    transaction6.addInput(input1);
    transaction6.addInput(input3);
    transaction6.addOutput(output2);
    transaction6.addOutput(output3);
    std::vector<blockchain::Transaction> transactions{transaction1, transaction2};

    // Calculate merkle root in header
    std::string prev_hash = signature::hash("prevheader");
    blockchain::Header header(prev_hash);
    header.calculateMerkleRoot(transactions);
    std::string merkle_root = header.getMerkleRoot();

    std::string digest = signature::hash(prev_hash + merkle_root);

    EXPECT_EQ(digest, header.getID());
}
