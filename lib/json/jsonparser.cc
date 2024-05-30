// TODO: include JSON library

#include "json/jsonparser.h"

using namespace std;
using namespace jsonparser;
using json = nlohmann::json;

// int JSONparser::JSONreadFileJson(int walletID,
//                                  std::vector<int>& value,
//                                  std::vector<int>& out_walletID) {
//     in.close();
//     return 1;
// }


 void jsonparser::from_json(const json& j, TransInfo& t) {
     j.at("walletID").get_to(t.walletid_);
        for (auto &value : j.at("ins")) {
            t.in_txID_list_.push_back(value.at("txID"));
                t.in_outIDX_list_.push_back(value.at("outIDX"));
        }
        for (auto &value : j.at("outs")) {
            t.out_value_list_.push_back(value.at("value"));
                t.out_walletID_list_.push_back(value.at("outwalletID"));
        }
}
    

void jsonparser::to_json(json& j, const TransInfo& s) {

        /*j["walletID"] = s.walletid_;
        //TODO: not consistent yet

        for (auto & t : s.value_list_) {
            j["value"].push_back(t);
        }

        for (auto & t : s.out_walletID_list_) {
            j["out_walletID_list"].push_back(t);
        }*/

    }

