#include "blockchain/input.h"

using namespace blockchain;

Input::Input(const chat::Transaction::Input& proto_input) {
    txID_ = proto_input.txid();
    outputIndex_ = proto_input.outputindex();
}

std::string Input::to_string() {
    json j;
    j["txID"] = txID_;
    j["outputIndex"] = outputIndex_;
    return j.dump();
}

void Input::from_string(std::string input_string) {
    json j = json::parse(input_string);
    txID_ = j.at("txID");
    outputIndex_ = j.at("outputIndex");
}
