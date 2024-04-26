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

    unsigned char signat[4] = {1, 2, 3, 4};
    transaction1.setSenderSig(signat);
    transaction2.setSenderSig(signat);

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);
    block.finalize();

    // Create UTXOlist
    blockchain::UTXO utxo1(101, 11, output1);
    blockchain::UTXO utxo2(102, 12, output2);
    blockchain::UTXO utxo3(103, 13, output3);
    std::vector<blockchain::UTXO> utxolist = {utxo1, utxo2, utxo3};

    // Create messages
    blockchain::Message messageBlock(blockchain::MsgType::ECHO, block, 1002, 1);
    blockchain::Message messageTx(blockchain::MsgType::SEND, transaction1, 2003, 1);
    blockchain::Message messageUTXO(blockchain::MsgType::SEND, utxolist, 3001, 1);

    // Check getters
    // messageBlock
    EXPECT_EQ(blockchain::MsgType::ECHO, messageBlock.getType());
    EXPECT_EQ(blockchain::MsgContent::BLOCK, messageBlock.getMessageContentType());
    EXPECT_EQ(block, messageBlock.getBlock());
    EXPECT_EQ(1002, messageBlock.getSenderId());
    EXPECT_EQ(1, messageBlock.getRound());
    EXPECT_FALSE(messageBlock.isEmpty());
    // messageTx
    EXPECT_EQ(blockchain::MsgType::SEND, messageTx.getType());
    EXPECT_EQ(blockchain::MsgContent::TRANSACTION, messageTx.getMessageContentType());
    EXPECT_EQ(transaction1, messageTx.getTransaction());
    EXPECT_EQ(2003, messageTx.getSenderId());
    EXPECT_EQ(1, messageTx.getRound());
    EXPECT_FALSE(messageTx.isEmpty());
    // messageUTXO
    EXPECT_EQ(blockchain::MsgType::SEND, messageUTXO.getType());
    EXPECT_EQ(blockchain::MsgContent::UTXOLIST, messageUTXO.getMessageContentType());
    EXPECT_EQ(utxolist, messageUTXO.getUTXOlist());
    EXPECT_EQ(3001, messageUTXO.getSenderId());
    EXPECT_EQ(1, messageUTXO.getRound());
    EXPECT_FALSE(messageUTXO.isEmpty());

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

    unsigned char signat[4] = {1, 2, 3, 4};
    transaction1.setSenderSig(signat);
    transaction2.setSenderSig(signat);

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);
    block.finalize();

    // Create UTXOlist
    blockchain::UTXO utxo1(101, 11, output1);
    blockchain::UTXO utxo2(102, 12, output2);
    blockchain::UTXO utxo3(103, 13, output3);
    std::vector<blockchain::UTXO> utxolist = {utxo1, utxo2, utxo3};

    // Create messages
    blockchain::Message messageBlock(blockchain::MsgType::ECHO, block, 1002, 1);
    blockchain::Message messageTx(blockchain::MsgType::SEND, transaction1, 2003, 1);
    blockchain::Message messageUTXO(blockchain::MsgType::SEND, utxolist, 3001, 1);

    // Convert
    chat::Message proto_msg_block = messageBlock.toProtoMessage();
    blockchain::Message msg_block_rec(proto_msg_block);
    chat::Message proto_msg_tx = messageTx.toProtoMessage();
    blockchain::Message msg_tx_rec(proto_msg_tx);
    chat::Message proto_msg_utxo = messageUTXO.toProtoMessage();
    blockchain::Message msg_utxo_rec(proto_msg_utxo);

    // Check getters
    // block
    EXPECT_EQ(blockchain::MsgType::ECHO, msg_block_rec.getType());
    EXPECT_EQ(blockchain::MsgContent::BLOCK, msg_block_rec.getMessageContentType());
    EXPECT_EQ(block, msg_block_rec.getBlock());
    EXPECT_EQ(1002, msg_block_rec.getSenderId());
    EXPECT_EQ(1, msg_block_rec.getRound());
    EXPECT_FALSE(msg_block_rec.isEmpty());
    // transaction
    EXPECT_EQ(blockchain::MsgType::SEND, msg_tx_rec.getType());
    EXPECT_EQ(blockchain::MsgContent::TRANSACTION, msg_tx_rec.getMessageContentType());
    EXPECT_EQ(transaction1, msg_tx_rec.getTransaction());
    EXPECT_EQ(2003, msg_tx_rec.getSenderId());
    EXPECT_EQ(1, msg_tx_rec.getRound());
    EXPECT_FALSE(msg_tx_rec.isEmpty());
    // utxo
    EXPECT_EQ(blockchain::MsgType::SEND, msg_utxo_rec.getType());
    EXPECT_EQ(blockchain::MsgContent::UTXOLIST, msg_utxo_rec.getMessageContentType());
    EXPECT_EQ(utxolist, msg_utxo_rec.getUTXOlist());
    EXPECT_EQ(3001, msg_utxo_rec.getSenderId());
    EXPECT_EQ(1, msg_utxo_rec.getRound());
    EXPECT_FALSE(msg_utxo_rec.isEmpty());
}
