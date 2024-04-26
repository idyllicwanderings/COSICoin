#include "blockchain/output.h"

using namespace blockchain;

Output::Output(const chat::Transaction::Output& proto_output) {
    value_ = proto_output.value();
    receiverID_ = proto_output.receiverid();
}

// chat::Output Output::toProtoOutput() const {
//     chat::Output proto_output;

//     proto_output.set_value(value_);
//     proto_output.set_receiverid(receiverID_);

//     return proto_output;
// }