#include "signature/hash.h"

#include <gtest/gtest.h>

#include <random>
#include <string>
#include <thread>

#define KEY_LEN_ 256

using namespace signature;

std::string generate_random_string(int length, std::string check_string = "") {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    static const int charsetSize = sizeof(charset) - 1;

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, charsetSize - 1);

    std::string randomString;
    randomString.reserve(length);

    for (int i = 0; i < length; ++i) {
        randomString += charset[distribution(generator)];
    }

    while (randomString == check_string) {
        randomString = "";
        for (int i = 0; i < length; ++i) {
            randomString += charset[distribution(generator)];
        }
    }

    return randomString;
}

std::bitset<KEY_LEN_> generate_random_bitset() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 1);
    std::bitset<KEY_LEN_> randomBitset;
    for (int i = 0; i < KEY_LEN_; ++i) {
        randomBitset[i] = distribution(generator);
    }
    return randomBitset;
}

bool test_non_zero(std::vector<std::bitset<KEY_LEN_>> key) {
    for (std::bitset<KEY_LEN_> bs : key) {
        if (bs.any()) {
            return true;
        }
    }
    return false;
}

bool test_non_zero(std::vector<std::string> key) {
    for (std::string k : key) {
        std::bitset<KEY_LEN_> bs = string_to_bitset(k);
        if (bs.any()) {
            return true;
        }
    }
    return false;
}

TEST(HashTest, StringConvert) {
    // bitset -> string -> bitset
    std::bitset<KEY_LEN_> bs = generate_random_bitset();
    EXPECT_EQ(KEY_LEN_, bs.size());
    EXPECT_TRUE(bs.any());
    std::string bs_str = bitset_to_string(bs);
    EXPECT_EQ(KEY_LEN_ / 8, bs_str.size());
    std::bitset<KEY_LEN_> bs_bc = string_to_bitset(bs_str);
    EXPECT_EQ(KEY_LEN_, bs_bc.size());
    EXPECT_EQ(bs, bs_bc);

    // string -> bitset -> string
    std::string str = generate_random_string(KEY_LEN_ / 8);
    EXPECT_EQ(KEY_LEN_ / 8, str.size());
    std::bitset<KEY_LEN_> str_bs = string_to_bitset(str);
    EXPECT_EQ(KEY_LEN_, str_bs.size());
    std::string str_bc = bitset_to_string(str_bs);
    EXPECT_EQ(KEY_LEN_ / 8, str_bc.size());
    EXPECT_EQ(str, str_bc);

    // hashed string -> bitset -> hashed string
    std::string hstr = hash(generate_random_string(KEY_LEN_ / 8)).substr(0, KEY_LEN_ / 8);
    EXPECT_EQ(KEY_LEN_ / 8, hstr.size());
    std::bitset<KEY_LEN_> hstr_bs = string_to_bitset(hstr);
    EXPECT_EQ(KEY_LEN_, hstr_bs.size());
    std::string hstr_bc = bitset_to_string(hstr_bs);
    EXPECT_EQ(KEY_LEN_ / 8, hstr_bc.size());
    EXPECT_EQ(hstr, hstr_bc);
}

TEST(SignatureTest, KeyGen) {
    // Key Gen
    Signature signature1 = Signature::getInstance();
    EXPECT_TRUE(signature1.getPublicKey().S0.empty());
    EXPECT_TRUE(signature1.getPublicKey().S1.empty());
    EXPECT_TRUE(signature1.getPrivateKey().S0.empty());
    EXPECT_TRUE(signature1.getPrivateKey().S1.empty());
    signature1.KeyGen();
    EXPECT_EQ(KEY_LEN_, signature1.getPublicKey().S0.size());
    EXPECT_EQ(KEY_LEN_, signature1.getPublicKey().S1.size());
    EXPECT_EQ(KEY_LEN_, signature1.getPrivateKey().S0.size());
    EXPECT_EQ(KEY_LEN_, signature1.getPrivateKey().S1.size());

    // Key Test
    SigKey public_key = signature1.getPublicKey();
    SigKey private_key = signature1.getPrivateKey();
    EXPECT_NE(public_key, private_key);
    for (int i = 0; i < KEY_LEN_; i++) {
        EXPECT_EQ(public_key.S0[i], hash(private_key.S0[i]));
        EXPECT_EQ(public_key.S1[i], hash(private_key.S1[i]));
    }
}

TEST(SigKeyTest, StringConvert) {
    // Key convert
    Signature signature1 = Signature::getInstance();
    signature1.KeyGen();
    SigKey key1 = signature1.getPublicKey();
    std::string string_key = SigKey_to_string(key1);
    SigKey key2 = SigKey_from_string(string_key);
    EXPECT_EQ(key1, key2);

    // Signature convert
    std::string m = bitset_to_string(generate_random_bitset());
    std::vector<std::string> sig1 = Sign(m, signature1.getPrivateKey());
    std::string sig_str = signature_to_json(sig1);
    std::vector<std::string> sig2 = json_to_string(sig_str);
    EXPECT_EQ(sig1, sig2);
}

TEST(HashTest, SameInputString) {
    // Short input
    std::string inputA = "hello";
    std::string inputB = "bye";
    std::string result1 = hash(inputA);
    std::string result2 = hash(inputA);
    std::string result3 = hash(inputB);
    EXPECT_EQ(result1, result2);
    EXPECT_NE(result1, result3);

    // 256 bit input
    inputA = generate_random_string(32);
    inputB = generate_random_string(32, inputA);
    result1 = hash(inputA);
    result2 = hash(inputA);
    result3 = hash(inputB);
    EXPECT_EQ(result1, result2);
    EXPECT_NE(result1, result3);

    // 512 bit input
    inputA = generate_random_string(64);
    inputB = generate_random_string(64, inputA);
    result1 = hash(inputA);
    result2 = hash(inputA);
    result3 = hash(inputB);
    EXPECT_EQ(result1, result2);
    EXPECT_NE(result1, result3);

    // 1024 bit input
    inputA = generate_random_string(128);
    inputB = generate_random_string(128, inputA);
    result1 = hash(inputA);
    result2 = hash(inputA);
    result3 = hash(inputB);
    EXPECT_EQ(result1, result2);
    EXPECT_NE(result1, result3);
}

void hash_test_threads(std::vector<std::string>& inputs, std::vector<std::string>& outputs) {
    for (int i = 0; i < 100; i++) {
        outputs.push_back(hash(inputs[i]));
    }
}

TEST(HashTest, Threads) {
    std::vector<std::string> inputs;
    std::vector<std::string> outputs1;
    std::vector<std::string> outputs2;
    for (int i = 0; i < 100; i++) {
        inputs.push_back(generate_random_string(52));
    }
    for (int i = 0; i < 100; i++) {
        outputs1.push_back(hash(inputs[i]));
    }
    std::thread thr(hash_test_threads, std::ref(inputs), std::ref(outputs2));
    thr.join();
    EXPECT_EQ(outputs1, outputs2);
}

TEST(SignatureTest, Signing) {
    Signature signature1 = Signature::getInstance();
    Signature signature2 = Signature::getInstance();
    signature1.KeyGen();
    signature2.KeyGen();
    EXPECT_NE(signature1.getPublicKey(), signature2.getPublicKey());
    EXPECT_NE(signature1.getPrivateKey(), signature2.getPrivateKey());

    // Sign
    std::string ma = bitset_to_string(generate_random_bitset());
    std::string mb = hash(bitset_to_string(generate_random_bitset()));
    EXPECT_NE(ma, mb);
    std::vector<std::string> sig1a1 = Sign(ma, signature1.getPrivateKey());
    std::vector<std::string> sig2a1 = Sign(ma, signature2.getPrivateKey());
    std::vector<std::string> sig1b1 = Sign(mb, signature1.getPrivateKey());
    std::vector<std::string> sig1a2 = Sign(ma, signature1.getPrivateKey());
    EXPECT_TRUE(test_non_zero(sig1a1));
    EXPECT_TRUE(test_non_zero(sig2a1));
    EXPECT_TRUE(test_non_zero(sig1b1));
    EXPECT_TRUE(test_non_zero(sig1a2));
    EXPECT_NE(sig1a1, sig2a1);  // checks if signed with different keys results in different signature
    EXPECT_NE(sig1a1, sig1b1);  // checks if different messages result in different signature
    EXPECT_EQ(sig1a1, sig1a2);  // checks if same message & same key result in same signature

    // Verify sig1a1
    EXPECT_EQ(1, Verify(ma, sig1a1, signature1.getPublicKey()));  // correct signature
    EXPECT_EQ(0, Verify(mb, sig1a1, signature1.getPublicKey()));  // wrong message
    EXPECT_EQ(0, Verify(ma, sig1b1, signature1.getPublicKey()));  // wrong signature, same key
    EXPECT_EQ(0, Verify(ma, sig2a1, signature1.getPublicKey()));  // wrong signature, wrong key
    EXPECT_EQ(0, Verify(ma, sig1a1, signature2.getPublicKey()));  // wrong public key

    // Verify sig2a1
    EXPECT_EQ(1, Verify(ma, sig2a1, signature2.getPublicKey()));  // correct signature

    // Verify sig1b1
    EXPECT_EQ(1, Verify(mb, sig1b1, signature1.getPublicKey()));  // correct signature

    // Verify sig1a2
    EXPECT_EQ(1, Verify(ma, sig1a2, signature1.getPublicKey()));  // correct signature
}
