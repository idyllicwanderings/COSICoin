
#ifndef COSICOIN_JSON_H
#define COSICOIN_JSON_H

#include "json/json.hpp"
#include <iostream>
#include <string>
#include <fstream>

using json = nlohmann::json;

namespace jsonparser {

    class Parser {
        public:
            Parser(std::string filename): filename_(filename) {
                jf = json::parse(filename_);
            }

            void output(std::string outname, json& j) {
                 std::ofstream file("test.json");
                 file << j;
            }
            
            json jf;

        private:
            const std::string filename_ = "tx.json";
    };

    
    class TransInfo {
        public:
            TransInfo();
            int walletid_;
            std::vector<int>& value_list_;
            std::vector<int>& out_walletID_list_;
    };


    // For an Trans_info object
    void from_json(const json& j, TransInfo& t) {
        j.at("wallet_id").get_to(t.walletid_);
    
        for (auto &value : j["value_list"]) {
            t.value_list_.push_back(value);
        }

        for (auto &value : j["out_walletID_list"]) {
            t.out_walletID_list_.push_back(value);
        }

    }
    
    void to_json(json& j, const TransInfo& s) {

        j = json{ {"wallet_id", s.walletid_} };

        for (auto & t : s.value_list_) {
            j["value_list"].push_back(t);
        }

        for (auto & t : s.out_walletID_list_) {
            j["out_walletID_list"].push_back(t);
        }

    }


}

#endif