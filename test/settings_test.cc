#include "config/settings.h"

#include <gtest/gtest.h>

TEST(SettingsTest, Getters) {
    config::AddressInfo val0;
    val0.id = 0;
    val0.address = "192.168.0.5";
    val0.port = 50000;
    val0.faulty = false;
    config::AddressInfo val1;
    val1.id = 1;
    val1.address = "192.168.0.54";
    val1.port = 50001;
    val1.faulty = true;
    config::AddressInfo val2;
    val2.id = 2;
    val2.address = "192.168.0.27";
    val2.port = 50002;
    val2.faulty = false;
    config::AddressInfo val3;
    val3.id = 3;
    val3.address = "192.168.0.28";
    val3.port = 50003;
    val3.faulty = false;
    std::vector<config::AddressInfo> validators{val0, val1, val2, val3};

    std::vector<uint32_t> wallets{0, 1, 2, 3};

    config::Settings settings1("settings.json");
    config::Settings settings2(validators, 0, wallets, 2, -1);

    EXPECT_EQ(val0, settings1.getLeaderInfo());
    EXPECT_EQ(val0, settings1.getValidatorInfo(0));
    EXPECT_EQ(val1, settings1.getValidatorInfo(1));
    EXPECT_EQ(val2, settings1.getValidatorInfo(2));
    EXPECT_EQ(val3, settings1.getValidatorInfo(3));
    EXPECT_EQ(validators, settings1.getValidators());
    EXPECT_EQ(val0, settings2.getLeaderInfo());
    EXPECT_EQ(val0, settings2.getValidatorInfo(0));
    EXPECT_EQ(val1, settings2.getValidatorInfo(1));
    EXPECT_EQ(val2, settings2.getValidatorInfo(2));
    EXPECT_EQ(val3, settings2.getValidatorInfo(3));
    EXPECT_EQ(validators, settings2.getValidators());

    std::vector<std::string> val_addresses{"192.168.0.5:50000", "192.168.0.54:50001", "192.168.0.27:50002", "192.168.0.28:50003"};

    EXPECT_EQ(val_addresses, settings1.getValidatorAdresses());
    EXPECT_EQ("192.168.0.5:50000", settings1.getLeaderAddress());
    EXPECT_EQ(val_addresses, settings2.getValidatorAdresses());
    EXPECT_EQ("192.168.0.5:50000", settings2.getLeaderAddress());

    EXPECT_EQ(wallets, settings1.getWallets());
    EXPECT_EQ(wallets, settings2.getWallets());

    EXPECT_EQ(4, settings1.getTotalNumberOfValidators());
    EXPECT_EQ(4, settings2.getTotalNumberOfValidators());
    EXPECT_EQ(1, settings1.getNumberOfFaultyValidators());
    EXPECT_EQ(1, settings2.getNumberOfFaultyValidators());

    EXPECT_EQ(0, settings1.getLeaderID());
    EXPECT_EQ(2, settings1.getMyValidatorID());
    EXPECT_EQ(-1, settings1.getMyWalletID());
    EXPECT_EQ(0, settings2.getLeaderID());
    EXPECT_EQ(2, settings2.getMyValidatorID());
    EXPECT_EQ(-1, settings2.getMyWalletID());

    EXPECT_EQ(val2, settings1.getMyValidatorInfo());
    EXPECT_EQ(val2, settings2.getMyValidatorInfo());
    EXPECT_EQ("192.168.0.27:50002", settings1.getMyValidatorAddress());
    EXPECT_EQ("192.168.0.27:50002", settings2.getMyValidatorAddress());
}
