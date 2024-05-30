#include "config/settings.h"

using namespace config;

void Settings::from_json(const json& j, config::Settings& s) {
    // Get leader_id from the json
    j.at("leaderID").get_to(s.leader_id_);

    // Get validators from the json
    validators_.clear();
    for (const auto& element : j.at("validators")) {
        config::AddressInfo validator_info;
        validator_info.id = element.at("id");
        validator_info.address = element.at("address");
        validator_info.port = element.at("port");
        validator_info.faulty = element.at("faulty");
        s.validators_[validator_info.id] = validator_info;
    }

    // Get wallets from the json
    wallets_.clear();
    for (const auto& element : j.at("wallets")) {
        wallets_.push_back(element);
    }

    // Get my_validator_id from the json
    j.at("myValidatorID").get_to(s.my_validator_id_);

    // Get my_wallet_id from the json
    j.at("myWalletID").get_to(s.my_wallet_id_);
}

void Settings::to_json(json& j, const config::Settings& s) {
    // Set leader_id in the json
    j["leaderID"] = s.leader_id_;

    // Set validators in the json
    for (const auto& validator : s.validators_) {
        j["validators"].push_back({{"id", validator.second.id},
                                   {"address", validator.second.address},
                                   {"port", validator.second.port},
                                   {"faulty", validator.second.faulty}});
    }

    // Set wallets in the json
    j["wallets"] = wallets_;

    // Set my_validator_id in the json
    j["myValidatorID"] = s.my_validator_id_;

    // Set my_wallet_id in the json
    j["myWalletID"] = s.my_wallet_id_;
}

std::string Settings::to_address(std::string ip_addr, uint32_t port) {
    return ip_addr + ':' + std::to_string(port);
}

Settings::Settings(std::vector<AddressInfo> validators, uint32_t leader_id, std::vector<uint32_t> wallets, uint32_t my_validator_id, uint32_t my_wallet_id) {
    leader_id_ = leader_id;
    my_validator_id_ = my_validator_id;
    my_wallet_id_ = my_wallet_id;
    for (const auto& val : validators) {
        validators_[val.id] = val;
    }
    wallets_ = wallets;
}

Settings::Settings(std::string settings_file) {
    std::ifstream i(settings_file);
    if (!i.is_open()) {
        throw std::runtime_error("Could not open settings file: " + settings_file);
    }

    json j;
    i >> j;

    from_json(j, *this);

    i.close();
}

std::vector<AddressInfo> Settings::getValidators() {
    std::vector<AddressInfo> validatorList;
    for (const auto& validator : validators_) {
        validatorList.push_back(validator.second);
    }
    return validatorList;
}

std::vector<std::string> Settings::getValidatorAdresses() {
    std::vector<std::string> adresses;
    for (const auto& validator : validators_) {
        std::string addr = to_address(validator.second.address, validator.second.port);
        adresses.push_back(addr);
    }
    return adresses;
}

std::string Settings::getLeaderAddress() {
    AddressInfo leader = getLeaderInfo();
    return to_address(leader.address, leader.port);
}

int Settings::getNumberOfFaultyValidators() {
    int faulty = 0;
    for (const auto& validator : validators_) {
        if (validator.second.faulty) {
            faulty++;
        }
    }
    return faulty;
}

config::AddressInfo Settings::getMyValidatorInfo() {
    // Check if my_validator_id_ is in validators_
    if (validators_.find(my_validator_id_) == validators_.end()) {
        return config::AddressInfo();
    }
    return validators_[my_validator_id_];
}

std::string Settings::getMyValidatorAddress() {
    AddressInfo my_validator = getMyValidatorInfo();
    return to_address(my_validator.address, my_validator.port);
}
