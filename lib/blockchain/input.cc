#include "blockchain/input.h"

using namespace blockchain;

Input::Input(const chat::Transaction::Input& proto_input) {
    txID_ = proto_input.txid();
    outputIndex_ = proto_input.outputindex();
}

// chat::Input Input::toProtoInput() const {
//     chat::Input proto_input;

//     proto_input.set_txid(txID_);
//     proto_input.set_outputindex(outputIndex_);

//     return proto_input;
// }