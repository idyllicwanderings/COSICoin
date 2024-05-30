#include "blockchain/message.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

#include "blockchain/output.h"
#include "blockchain/utxo.h"

TEST(MessageTest, Getters) {
    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, 1001);
    blockchain::Output output2(20, 1002);
    blockchain::Output output3(10, 1002);

    // Create transactions
    blockchain::Transaction transaction1(3001);
    blockchain::Transaction transaction2(3002);
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction2.addInput(input3);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addOutput(output3);

    // add signature
    signature::Signature signature = signature::Signature::getInstance();
    signature.KeyGen();
    std::vector<std::string> signat1 = signature::Sign(transaction1.getDigest(), signature.getPrivateKey());
    std::vector<std::string> signat2 = signature::Sign(transaction2.getDigest(), signature.getPrivateKey());
    transaction1.setSenderSig(signat1);
    transaction2.setSenderSig(signat2);
    transaction1.setPublicKey(signature.getPublicKey());

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);
    block.finalize();

    // Create messages
    blockchain::Message messageBlock(blockchain::MsgType::ECHO, block, 1002, 1);

    // Check getters
    // messageBlock
    EXPECT_EQ(blockchain::MsgType::ECHO, messageBlock.getType());
    EXPECT_EQ(block, messageBlock.getBlock());
    EXPECT_EQ(1002, messageBlock.getSenderId());
    EXPECT_EQ(1, messageBlock.getRound());
    EXPECT_FALSE(messageBlock.isEmpty());

    // Change type
    messageBlock.setType(blockchain::MsgType::READY);
    EXPECT_EQ(blockchain::MsgType::READY, messageBlock.getType());
}

TEST(MessageTest, ProtoConvert) {
    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, 1001);
    blockchain::Output output2(20, 1002);
    blockchain::Output output3(10, 1002);

    // Create transactions
    blockchain::Transaction transaction1(3001);
    blockchain::Transaction transaction2(3002);
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction2.addInput(input3);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addOutput(output3);

    // add signature
    signature::Signature signature = signature::Signature::getInstance();
    signature.KeyGen();
    std::vector<std::string> signat1 = signature::Sign(transaction1.getDigest(), signature.getPrivateKey());
    std::vector<std::string> signat2 = signature::Sign(transaction2.getDigest(), signature.getPrivateKey());
    transaction1.setSenderSig(signat1);
    transaction2.setSenderSig(signat2);
    transaction1.setPublicKey(signature.getPublicKey());
    transaction2.setPublicKey(signature.getPublicKey());

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);
    block.finalize();

    // Sign block
    signature::Signature signature1 = signature::Signature::getInstance();
    signature::Signature signature2 = signature::Signature::getInstance();
    signature1.KeyGen();
    signature2.KeyGen();
    block.sign(signature1.getPrivateKey(), 26, signature2.getPublicKey());

    // Create messages
    blockchain::Message messageBlock(blockchain::MsgType::ECHO, block, 1002, 1);

    // Convert
    chat::Message proto_msg_block = messageBlock.toProtoMessage();
    blockchain::Message msg_block_rec(proto_msg_block);

    // Check getters
    // block
    EXPECT_EQ(blockchain::MsgType::ECHO, msg_block_rec.getType());
    EXPECT_EQ(block, msg_block_rec.getBlock());
    EXPECT_EQ(1002, msg_block_rec.getSenderId());
    EXPECT_EQ(1, msg_block_rec.getRound());
    EXPECT_FALSE(msg_block_rec.isEmpty());
}
