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

    // Sign block
    signature::Signature signature1 = signature::Signature::getInstance();
    signature::Signature signature2 = signature::Signature::getInstance();
    signature1.KeyGen();
    signature2.KeyGen();
    std::string digest1 = block.getDigest();
    block.sign(signature1.getPrivateKey(), 26, signature2.getPublicKey());
    std::string digest2 = block.getDigest();
    EXPECT_NE(digest1, digest2);
    std::vector<std::string> block_sig = signature::Sign(digest2, signature1.getPrivateKey());
    EXPECT_EQ(1, signature::Verify(digest2, block_sig, signature1.getPublicKey()));

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
    std::string digest3 = block.getDigest();
    EXPECT_EQ(digest2, digest3);
    EXPECT_EQ(1, signature::Verify(block.getDigest(), block.getValidatorSignature(), signature1.getPublicKey()));
    EXPECT_TRUE(block.verifySignature(signature1.getPublicKey()));
    EXPECT_EQ(1, signature::Verify(digest3, block_sig, signature1.getPublicKey()));
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
    block.sign(signature1.getPrivateKey(), 26, signature2.getPublicKey());

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
    EXPECT_EQ(26, rec1_block.getValidatorID());
    EXPECT_EQ(signature2.getPublicKey(), rec1_block.getValidatorNextPublicKey());
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
    EXPECT_EQ(26, rec2_block.getValidatorID());
    EXPECT_EQ(signature2.getPublicKey(), rec2_block.getValidatorNextPublicKey());
}

TEST(BlockTest, StringConvert) {
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

    // Make block immutable
    block.finalize();

    // Sign block
    signature::Signature signature1 = signature::Signature::getInstance();
    signature::Signature signature2 = signature::Signature::getInstance();
    signature1.KeyGen();
    signature2.KeyGen();
    block.sign(signature1.getPrivateKey(), 26, signature2.getPublicKey());
    blockchain::BlockSignature block_sig;
    block_sig.validator_id = 26;
    block_sig.signature = block.getValidatorSignature();
    block_sig.public_key = signature1.getPublicKey();
    block_sig.round = 1;
    block.addValidatorSignature(block_sig);

    // Convert
    std::string str_block = block.to_string();
    blockchain::Block rec_block;
    rec_block.from_string(str_block);

    // Check outputs rec
    EXPECT_EQ(2, rec_block.getID());
    EXPECT_EQ("prevblock", rec_block.getHeader().getPrevBlockDigest());
    std::vector<blockchain::Transaction> transactions = rec_block.getTransactions();
    EXPECT_EQ(input1, transactions[0].getInputAt(0));
    EXPECT_EQ(input2, transactions[0].getInputAt(1));
    EXPECT_EQ(input3, transactions[1].getInputAt(0));
    EXPECT_EQ(output1, transactions[0].getOutputAt(0));
    EXPECT_EQ(output2, transactions[0].getOutputAt(1));
    EXPECT_EQ(output3, transactions[1].getOutputAt(0));
    EXPECT_TRUE(transactions[0] == transaction1);
    EXPECT_TRUE(transactions[1] == transaction2);
    EXPECT_EQ(26, rec_block.getValidatorID());
    EXPECT_EQ(signature2.getPublicKey(), rec_block.getValidatorNextPublicKey());
    EXPECT_EQ(1, block.getValidatorSignatures().size());
    EXPECT_EQ(block_sig, block.getValidatorSignatures()[0]);
    EXPECT_EQ(1, rec_block.getValidatorSignatures().size());
    EXPECT_EQ(block_sig, rec_block.getValidatorSignatures()[0]);
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
    blockchain::UTXOlist utxolist(utxo_vector);

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

    std::vector<uint32_t> wallets{1001, 1002, 1005};

    // Verify transactions
    EXPECT_TRUE(transaction1.checkSpendingConditions(utxolist, wallets));
    EXPECT_TRUE(transaction2.checkSpendingConditions(utxolist, wallets));

    // Create block
    blockchain::Block block(2, "prevblock");
    EXPECT_TRUE(block.verifyTxConsist(transaction1));
    block.addTransaction(transaction1);
    EXPECT_FALSE(block.verifyTxConsist(transaction1));
    EXPECT_TRUE(block.verifyTxConsist(transaction2));
    block.addTransaction(transaction2);
    EXPECT_FALSE(block.verifyTxConsist(transaction2));

    // Verify block
    EXPECT_TRUE(block.verify(utxolist, wallets));
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
    blockchain::UTXOlist utxolist(utxo_vector);

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

    std::vector<uint32_t> wallets{1001, 1002, 1005};

    // Verify transactions
    EXPECT_FALSE(transaction1.checkSpendingConditions(utxolist, wallets));
    EXPECT_TRUE(transaction2.checkSpendingConditions(utxolist, wallets));

    // Create block
    blockchain::Block block(2, "prevblock");
    EXPECT_TRUE(block.verifyTxConsist(transaction1));
    block.addTransaction(transaction1);
    EXPECT_FALSE(block.verifyTxConsist(transaction1));
    EXPECT_TRUE(block.verifyTxConsist(transaction2));
    block.addTransaction(transaction2);
    EXPECT_FALSE(block.verifyTxConsist(transaction2));

    // Verify block
    EXPECT_FALSE(block.verify(utxolist, wallets));
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
    blockchain::UTXOlist utxolist(utxo_vector);

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

    // add signature
    signature::Signature signature = signature::Signature::getInstance();
    signature.KeyGen();
    std::vector<std::string> signat1 = signature::Sign(transaction1.getDigest(), signature.getPrivateKey());
    std::vector<std::string> signat2 = signature::Sign(transaction2.getDigest(), signature.getPrivateKey());
    transaction1.setSenderSig(signat1);
    transaction2.setSenderSig(signat2);
    transaction1.setPublicKey(signature.getPublicKey());

    std::vector<uint32_t> wallets{1001, 1002, 1005};

    // Verify transactions
    EXPECT_TRUE(transaction1.checkSpendingConditions(utxolist, wallets));
    EXPECT_TRUE(transaction2.checkSpendingConditions(utxolist, wallets));

    // Create block
    blockchain::Block block(2, "prevblock");
    EXPECT_TRUE(block.verifyTxConsist(transaction1));
    block.addTransaction(transaction1);
    EXPECT_FALSE(block.verifyTxConsist(transaction1));
    EXPECT_FALSE(block.verifyTxConsist(transaction2));
    block.addTransaction(transaction2);
    EXPECT_FALSE(block.verifyTxConsist(transaction2));

    // Verify block
    EXPECT_FALSE(block.verify(utxolist, wallets));
}
