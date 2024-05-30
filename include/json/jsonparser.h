
#ifndef COSICOIN_JSON_H
#define COSICOIN_JSON_H

#include "json.hpp"

#include <iostream>
#include <string>
#include <fstream>

using json = nlohmann::json;

namespace jsonparser {

    class Parser {
        public:
            Parser(): filename_("../../tx.json") {
                std::ifstream i(filename_);
                if (!i.is_open()) {
                throw std::runtime_error("Tx file oppen failed" + filename_); }
                i >> jf;
                //std::ifstream i(filename_);
                //jf = json::parse(i);
                i.close();
            }
            
            Parser(std::string filename): filename_(filename) {
                std::ifstream i("../../tx/" + filename_);
                if (!i.is_open()) {
                throw std::runtime_error("Tx file oppen failed" + filename_); }
                i >> jf;
                //std::ifstream i(filename_);
                //jf = json::parse(i);
                i.close();
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
            TransInfo() {;}
            uint32_t walletid_;
            std::vector<uint64_t> out_value_list_;
            std::vector<uint32_t> out_walletID_list_;
            
            std::vector<uint64_t> in_outIDX_list_;
            std::vector<uint32_t> in_txID_list_;
            
    };
    
    void from_json(const json& j, TransInfo& t);

    
    void to_json(json& j, const TransInfo& s);





}

#endif
