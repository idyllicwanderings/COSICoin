#include "blockchain/output.h"

using namespace blockchain;

Output::Output(const chat::Transaction::Output& proto_output) {
    value_ = proto_output.value();
    receiverID_ = proto_output.receiverid();
}

std::string Output::to_string() {
    json j;
    j["value"] = value_;
    j["receiverID"] = receiverID_;
    return j.dump();
}

void Output::from_string(std::string output_string) {
    json j = json::parse(output_string);
    value_ = j.at("value");
    receiverID_ = j.at("receiverID");
}
