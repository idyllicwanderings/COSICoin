#ifndef COSICOIN_SETTINGS_H
#define COSICOIN_SETTINGS_H

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "json/json.hpp"

using json = nlohmann::json;

namespace config {
// Struct to store info of one validator
struct AddressInfo {
    uint32_t id;
    std::string address;
    int port;
    bool faulty = false;

    friend bool operator==(const AddressInfo& lhs, const AddressInfo& rhs) {
        return lhs.id == rhs.id && lhs.address == rhs.address && lhs.port == rhs.port && lhs.faulty == rhs.faulty;
    }
};

// Class to get settings
class Settings {
   public:
    Settings(std::string settings_file);
    Settings(std::vector<AddressInfo> validators, uint32_t leader_id, std::vector<uint32_t> wallets, uint32_t my_validator_id, uint32_t my_wallet_id);

    // Returns a AddressInfo struct containing the info about the leader
    config::AddressInfo getLeaderInfo() { return validators_[leader_id_]; };

    // Returns a AddressInfo struct containing the info about the validator with given id
    config::AddressInfo getValidatorInfo(uint32_t validator_id) { return validators_[validator_id]; };

    // Returns a list of the info of each validator
    std::vector<AddressInfo> getValidators();

    // Returns a list of validators address:port
    std::vector<std::string> getValidatorAdresses();

    // Returns address of the leader
    std::string getLeaderAddress();

    // Returns a list of the info of each wallet
    std::vector<uint32_t> getWallets() { return wallets_; };

    int getTotalNumberOfValidators() { return validators_.size(); };
    int getNumberOfFaultyValidators();

    uint32_t getLeaderID() { return leader_id_; };
    uint32_t getMyValidatorID() { return my_validator_id_; };
    uint32_t getMyWalletID() { return my_wallet_id_; };

    config::AddressInfo getMyValidatorInfo();

    std::string getMyValidatorAddress();

   private:
    uint32_t leader_id_;
    std::map<uint32_t, AddressInfo> validators_;
    std::vector<uint32_t> wallets_;
    uint32_t my_validator_id_;
    uint32_t my_wallet_id_;

    void from_json(const json& j, config::Settings& s);

    void to_json(json& j, const config::Settings& s);

    std::string to_address(std::string ip_addr, uint32_t port);
};

}  // namespace config
#endif
