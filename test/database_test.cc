#include "database/database.h"

#include <gtest/gtest.h>

#include <random>

#define KEY_LEN_ 256

int random_nb(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

blockchain::Block generate_block(std::string prevBlockDigest) {
    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, random_nb(3000, 3500));
    blockchain::Output output2(20, random_nb(3000, 3500));
    blockchain::Output output3(10, random_nb(3000, 3500));

    // Create transactions
    blockchain::Transaction transaction1(random_nb(1000, 2000));
    blockchain::Transaction transaction2(random_nb(1000, 2000));
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
    blockchain::Block block(random_nb(0, 100), prevBlockDigest);
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);
    block.finalize();

    // Sign block
    signature::Signature signature1 = signature::Signature::getInstance();
    signature::Signature signature2 = signature::Signature::getInstance();
    signature1.KeyGen();
    signature2.KeyGen();
    block.sign(signature1.getPrivateKey(), 26, signature2.getPublicKey());

    // Add validator signature
    blockchain::BlockSignature block_sig;
    block_sig.validator_id = 26;
    block_sig.signature = block.getValidatorSignature();
    block_sig.public_key = signature1.getPublicKey();
    block_sig.round = 1;
    block.addValidatorSignature(block_sig);

    return block;
}

TEST(DatabaseTest, SaveLoad) {
    std::remove("database.json");
    std::remove("db_tmp.json");

    database::Database db("database.json");
    std::vector<std::string> digests;
    EXPECT_TRUE(db.getBlockDigests().empty());

    // Add block1
    blockchain::Block block1 = generate_block("-");
    EXPECT_EQ(1, block1.getValidatorSignatures().size());
    std::string digest1 = block1.getDigest();
    digests.push_back(digest1);

    std::cout << "block 1 validatorsigs size: " << block1.getValidatorSignatures().size() << std::endl;
    EXPECT_EQ(0, db.addBlock(block1));
    EXPECT_EQ(digests, db.getBlockDigests());
    EXPECT_EQ(1, db.getBlock(digest1).getValidatorSignatures().size());
    EXPECT_EQ(block1, db.getBlock(digest1));

    database::Database tmp_db("database.json");
    EXPECT_EQ(digests, tmp_db.getBlockDigests());
    EXPECT_EQ(block1, tmp_db.getBlock(digest1));

    // Add block2
    blockchain::Block block2 = generate_block(digest1);
    std::string digest2 = block2.getDigest();
    digests.push_back(digest2);

    EXPECT_EQ(0, db.addBlock(block2));
    EXPECT_EQ(digests, db.getBlockDigests());
    EXPECT_EQ(block2, db.getBlock(digest2));

    tmp_db.load("database.json");
    EXPECT_EQ(digests, tmp_db.getBlockDigests());
    EXPECT_EQ(block1, tmp_db.getBlock(digest1));
    EXPECT_EQ(block2, tmp_db.getBlock(digest2));

    // Add block3
    blockchain::Block block3 = generate_block(digest2);
    std::string digest3 = block3.getDigest();
    digests.push_back(digest3);

    EXPECT_EQ(0, db.addBlock(block3));
    EXPECT_EQ(digests, db.getBlockDigests());
    EXPECT_EQ(block3, db.getBlock(digest3));

    tmp_db.load("database.json");
    EXPECT_EQ(digests, tmp_db.getBlockDigests());
    EXPECT_EQ(block1, tmp_db.getBlock(digest1));
    EXPECT_EQ(block2, tmp_db.getBlock(digest2));
    EXPECT_EQ(block3, tmp_db.getBlock(digest3));

    // Add wrong blocks
    blockchain::Block block4 = generate_block("-");
    EXPECT_EQ(1, db.addBlock(block4));
    EXPECT_EQ(digests, db.getBlockDigests());

    blockchain::Block block5 = generate_block(digest1);
    EXPECT_EQ(1, db.addBlock(block5));
    EXPECT_EQ(digests, db.getBlockDigests());

    blockchain::Block block6 = generate_block(digest2);
    EXPECT_EQ(1, db.addBlock(block6));
    EXPECT_EQ(digests, db.getBlockDigests());
}