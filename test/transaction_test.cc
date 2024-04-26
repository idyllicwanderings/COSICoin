#include "blockchain/transaction.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

#include <iostream>

#include "blockchain/block.h"

TEST(TransactionTest, Getters) {
    // Create inputs
    blockchain::Input input1(1000, 10);
    blockchain::Input input2(1001, 2);
    blockchain::Input input3(1001, 7);

    // Create outputs
    blockchain::Output output1(6, 100);
    blockchain::Output output2(20, 101);
    blockchain::Output output3(10, 101);

    // Create transaction
    blockchain::Transaction transaction1(105);
    EXPECT_EQ(transaction1.getID(), 105);

    // Add inputs
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction1.addInput(input3);

    // Add outputs
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction1.addOutput(output3);

    // Check inputs
    EXPECT_EQ(input1, transaction1.getInputAt(0));
    EXPECT_EQ(input2, transaction1.getInputAt(1));
    EXPECT_EQ(input3, transaction1.getInputAt(2));

    // Check outputs
    EXPECT_EQ(output1, transaction1.getOutputAt(0));
    EXPECT_EQ(output2, transaction1.getOutputAt(1));
    EXPECT_EQ(output3, transaction1.getOutputAt(2));
}

TEST(TransactionTest, ProtoConvert) {
    // Create inputs
    blockchain::Input input1(1000, 10);
    blockchain::Input input2(1001, 2);
    blockchain::Input input3(1001, 7);

    // Create outputs
    blockchain::Output output1(6, 100);
    blockchain::Output output2(20, 101);
    blockchain::Output output3(10, 101);

    // Create transaction
    blockchain::Transaction transaction1(105);
    blockchain::Transaction transaction2(106);
    blockchain::Transaction transaction3(107);

    // Add inputs
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction2.addInput(input3);
    transaction3.addInput(input3);

    // Add outputs
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addOutput(output3);
    transaction3.addOutput(output3);

    // add signature
    unsigned char signat[4] = {1, 2, 3, 4};
    transaction1.setSenderSig(signat);
    transaction2.setSenderSig(signat);
    // no signature for transaction 3 to check error

    // Convert
    chat::Block block;
    chat::Block* p_block = &block;
    transaction1.toProtoTransaction(p_block->add_transaction());
    transaction2.toProtoTransaction(p_block->add_transaction());
    blockchain::Transaction transaction1_rec;
    blockchain::Transaction transaction2_rec;
    transaction1_rec.fromProtoTransaction(block.transaction(0));
    transaction2_rec.fromProtoTransaction(block.transaction(1));

    // Check ID
    EXPECT_EQ(105, transaction1_rec.getID());
    EXPECT_EQ(106, transaction2_rec.getID());
    // Check inputs
    EXPECT_EQ(input1, transaction1.getInputAt(0));
    EXPECT_EQ(input2, transaction1.getInputAt(1));
    EXPECT_EQ(input3, transaction2.getInputAt(0));

    EXPECT_EQ(input1, transaction1_rec.getInputAt(0));
    EXPECT_EQ(input2, transaction1_rec.getInputAt(1));
    EXPECT_EQ(input3, transaction2_rec.getInputAt(0));

    // Check outputs
    EXPECT_EQ(output1, transaction1.getOutputAt(0));
    EXPECT_EQ(output2, transaction1.getOutputAt(1));
    EXPECT_EQ(output3, transaction2.getOutputAt(0));

    EXPECT_EQ(output1, transaction1_rec.getOutputAt(0));
    EXPECT_EQ(output2, transaction1_rec.getOutputAt(1));
    EXPECT_EQ(output3, transaction2_rec.getOutputAt(0));

    // Check signature
    // check transaction equal
    EXPECT_EQ(transaction1, transaction1_rec);
    EXPECT_EQ(transaction2, transaction2_rec);

    // check no signature on transaction
    EXPECT_THROW(transaction3.toProtoTransaction(p_block->add_transaction()), std::runtime_error);
}

TEST(TransactionTest, CheckValidTransaction) {
    // Create inputs pointing to txID and outputindex of previous transactions, but is the input of new transaction
    blockchain::Input input1(104, 0); // txID of previous transaction, outputIndex
    blockchain::Input input2(104, 1);

    // Create outputs in new transaction 105
    blockchain::Output output1(6, 50); // value, receiverID
    blockchain::Output output2(20, 51);
    blockchain::Output output3(10, 52);

    // create outputs of previous transactions
    blockchain::Output output4(12, 100); // value, receiverID
    blockchain::Output output5(24, 101);

    // Create transaction
    blockchain::Transaction transaction1(105); // txID

    // Add inputs
    transaction1.addInput(input1);
    transaction1.addInput(input2);

    // Add outputs
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction1.addOutput(output3);

    // create utxo (so from outputs of previous transactions)
    blockchain::UTXO utxo1(104, 0, output4); // txID, outputIndex, output
    blockchain::UTXO utxo2(104, 1, output5);
    std::vector<blockchain::UTXO> utxo;
    utxo.push_back(utxo1);
    utxo.push_back(utxo2);
    // create public keys vector
    std::vector<uint32_t> publicKeys;
    publicKeys.push_back(50);
    publicKeys.push_back(51);
    publicKeys.push_back(52);
    EXPECT_TRUE(transaction1.checkSpendingConditions(utxo, publicKeys));

    // utxo vector with one missing output
    std::vector<blockchain::UTXO> utxo_missing;
    utxo_missing.push_back(utxo1);
    EXPECT_FALSE(transaction1.checkSpendingConditions(utxo_missing, publicKeys));

    // utxo vector with one output pointing to wrong receiver
    blockchain::Output output6(12, 53);
    blockchain::UTXO utxo_wrong_receiver(104, 0, output6);
    std::vector<blockchain::UTXO> utxo_wrong_receiver_vector;
    utxo_wrong_receiver_vector.push_back(utxo_wrong_receiver);
    EXPECT_FALSE(transaction1.checkSpendingConditions(utxo_wrong_receiver_vector, publicKeys));
}