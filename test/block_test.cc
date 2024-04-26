#include "blockchain/block.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <gtest/gtest.h>

#include "blockchain/output.h"
#include "signature/hash.h"

TEST(BlockTest, Getters) {
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
    block.sign(signature1, 26, signature2.getPublicKey());

    // Add validator signature
    unsigned char val_sig[KEY_LEN_];
    block.getValidatorSignature(val_sig);
    block.addValidatorSignature(26, val_sig);

    // Check exeptions
    EXPECT_THROW(block.addTransaction(transaction1), std::runtime_error);
    EXPECT_THROW(block.finalize(), std::runtime_error);

    // Check outputs
    EXPECT_EQ(2, block.getID());
    EXPECT_EQ("prevblock", block.getHeader().getPrevBlockDigest());
    std::vector<blockchain::Transaction> transactions = block.getTransactions();
    EXPECT_EQ(input1, transactions[0].getInputAt(0));
    EXPECT_EQ(input2, transactions[0].getInputAt(1));
    EXPECT_EQ(input3, transactions[1].getInputAt(0));
    EXPECT_EQ(output1, transactions[0].getOutputAt(0));
    EXPECT_EQ(output2, transactions[0].getOutputAt(1));
    EXPECT_EQ(output3, transactions[1].getOutputAt(0));
    EXPECT_EQ(26, block.getValidatorID());
    EXPECT_EQ(signature2.getPublicKey(), block.getValidatorNextPublicKey());
    unsigned char sig[KEY_LEN_];
    EXPECT_EQ(1, block.getValidatorSignatureFromList(sig, 26));
    std::string sig1 = block.charToString_(val_sig, KEY_LEN_);
    std::string sig2 = block.charToString_(sig, KEY_LEN_);
    EXPECT_EQ(sig1, sig2);
    EXPECT_EQ(0, block.getValidatorSignatureFromList(sig, 0));
}

TEST(BlockTest, CharStringConvert) {
    blockchain::Block block;

    // String to char to string
    std::string string1 = "Hello, world!";
    int len = string1.size();
    unsigned char charlist1[len];
    block.stringToChar_(charlist1, string1);
    std::string string2 = block.charToString_(charlist1, len);
    EXPECT_EQ(string1, string2);

    // Char to string to char
    unsigned char charlist2[4] = {1, 2, 3, 4};
    len = 4;
    std::string string3 = block.charToString_(charlist2, len);
    unsigned char charlist3[len];
    block.stringToChar_(charlist3, string3);
    for (int i = 0; i < len; i++) {
        EXPECT_EQ(charlist2[i], charlist3[i]);
    }
}

TEST(BlockTest, ProtoConvert) {
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

    // Check conversion error
    chat::Block proto_block;
    EXPECT_THROW(block.toProtoBlock(&proto_block), std::runtime_error);

    // Make block immutable
    block.finalize();

    // Sign block
    signature::Signature signature1 = signature::Signature::getInstance();
    signature::Signature signature2 = signature::Signature::getInstance();
    signature1.KeyGen();
    signature2.KeyGen();
    block.sign(signature1, 26, signature2.getPublicKey());

    // Convert
    block.toProtoBlock(&proto_block);
    blockchain::Block rec1_block(proto_block);
    blockchain::Block rec2_block;
    rec2_block.fromProtoBlock(proto_block);

    // Check outputs rec1
    EXPECT_EQ(2, rec1_block.getID());
    EXPECT_EQ("prevblock", rec1_block.getHeader().getPrevBlockDigest());
    std::vector<blockchain::Transaction> transactions = rec1_block.getTransactions();
    EXPECT_EQ(input1, transactions[0].getInputAt(0));
    EXPECT_EQ(input2, transactions[0].getInputAt(1));
    EXPECT_EQ(input3, transactions[1].getInputAt(0));
    EXPECT_EQ(output1, transactions[0].getOutputAt(0));
    EXPECT_EQ(output2, transactions[0].getOutputAt(1));
    EXPECT_EQ(output3, transactions[1].getOutputAt(0));
    EXPECT_TRUE(transactions[0] == transaction1);
    EXPECT_TRUE(transactions[1] == transaction2);
    EXPECT_EQ(26, block.getValidatorID());
    EXPECT_EQ(signature2.getPublicKey(), block.getValidatorNextPublicKey());
    // Check outputs rec2
    EXPECT_EQ(2, rec2_block.getID());
    EXPECT_EQ("prevblock", rec2_block.getHeader().getPrevBlockDigest());
    transactions = rec2_block.getTransactions();
    EXPECT_EQ(input1, transactions[0].getInputAt(0));
    EXPECT_EQ(input2, transactions[0].getInputAt(1));
    EXPECT_EQ(input3, transactions[1].getInputAt(0));
    EXPECT_EQ(output1, transactions[0].getOutputAt(0));
    EXPECT_EQ(output2, transactions[0].getOutputAt(1));
    EXPECT_EQ(output3, transactions[1].getOutputAt(0));
    EXPECT_TRUE(transactions[0] == transaction1);
    EXPECT_TRUE(transactions[1] == transaction2);
    EXPECT_EQ(26, block.getValidatorID());
    EXPECT_EQ(signature2.getPublicKey(), block.getValidatorNextPublicKey());
}

TEST(BlockTest, VerifyValid) {
    // Create inputs
    blockchain::Input input1(2001, 10);  // txID = 2001    outIndex = 10
    blockchain::Input input2(2002, 2);   // txID = 2002    outIndex = 2
    blockchain::Input input3(2003, 7);   // txID = 2003    outIndex = 7

    // Create outputs
    blockchain::Output output1(6, 1001);   // value = 6    recID = 1001
    blockchain::Output output2(20, 1002);  // value = 20   recID = 1002
    blockchain::Output output3(7, 1002);   // value = 7    recID = 1002

    // Create UTXO
    std::vector<blockchain::UTXO> utxo_vector;
    // UTXO(transactionID, outputIndex, output)
    utxo_vector.push_back(blockchain::UTXO(2001, 10, blockchain::Output(16, 1005)));
    utxo_vector.push_back(blockchain::UTXO(2002, 2, blockchain::Output(10, 1005)));
    utxo_vector.push_back(blockchain::UTXO(2003, 7, blockchain::Output(7, 1005)));

    // Create transactions
    blockchain::Transaction transaction1(3001);
    blockchain::Transaction transaction2(3002);
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction2.addInput(input3);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addOutput(output3);

    // Add signatures
    unsigned char signat[4] = {1, 2, 3, 4};
    transaction1.setSenderSig(signat);
    transaction2.setSenderSig(signat);

    std::vector<uint32_t> wallets{1001, 1002, 1005};

    // Verify transactions
    EXPECT_TRUE(transaction1.checkSpendingConditions(utxo_vector, wallets));
    EXPECT_TRUE(transaction2.checkSpendingConditions(utxo_vector, wallets));

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);

    // Verify block
    EXPECT_TRUE(block.verify(utxo_vector, wallets));
}

TEST(BlockTest, VerifyInvalidTransaction) {
    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, 1001);
    blockchain::Output output2(20, 1002);
    blockchain::Output output3(7, 1002);

    // Create UTXO
    std::vector<blockchain::UTXO> utxo_vector;
    utxo_vector.push_back(blockchain::UTXO(2001, 10, blockchain::Output(16, 1005)));
    utxo_vector.push_back(blockchain::UTXO(2002, 2, blockchain::Output(8, 1005)));
    utxo_vector.push_back(blockchain::UTXO(2003, 7, blockchain::Output(7, 1005)));

    // Create transactions
    blockchain::Transaction transaction1(3001);
    blockchain::Transaction transaction2(3002);
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction2.addInput(input3);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addOutput(output3);

    // Add signatures
    unsigned char signat[4] = {1, 2, 3, 4};
    transaction1.setSenderSig(signat);
    transaction2.setSenderSig(signat);

    std::vector<uint32_t> wallets{1001, 1002, 1005};

    // Verify transactions
    EXPECT_FALSE(transaction1.checkSpendingConditions(utxo_vector, wallets));
    EXPECT_TRUE(transaction2.checkSpendingConditions(utxo_vector, wallets));

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);

    // Verify block
    EXPECT_FALSE(block.verify(utxo_vector, wallets));
}

TEST(BlockTest, VerifyDublicateInputs) {
    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, 1001);
    blockchain::Output output2(20, 1002);
    blockchain::Output output3(23, 1002);

    // Create UTXO
    std::vector<blockchain::UTXO> utxo_vector;
    utxo_vector.push_back(blockchain::UTXO(2001, 10, blockchain::Output(16, 1005)));
    utxo_vector.push_back(blockchain::UTXO(2002, 2, blockchain::Output(10, 1005)));
    utxo_vector.push_back(blockchain::UTXO(2003, 7, blockchain::Output(7, 1005)));

    // Create transactions
    blockchain::Transaction transaction1(3001);
    blockchain::Transaction transaction2(3002);
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction2.addInput(input3);
    transaction2.addInput(input1);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addOutput(output3);

    // Add signatures
    unsigned char signat[4] = {1, 2, 3, 4};
    transaction1.setSenderSig(signat);
    transaction2.setSenderSig(signat);

    std::vector<uint32_t> wallets{1001, 1002, 1005};

    // Verify transactions
    EXPECT_TRUE(transaction1.checkSpendingConditions(utxo_vector, wallets));
    EXPECT_TRUE(transaction2.checkSpendingConditions(utxo_vector, wallets));

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);

    // Verify block
    EXPECT_FALSE(block.verify(utxo_vector, wallets));
}
