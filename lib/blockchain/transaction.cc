#include "blockchain/transaction.h"

using namespace blockchain;

bool Transaction::checkSpendingConditions(std::vector<UTXO>& utxo, std::vector<uint32_t>& publicKeys) {
    // Check if the transaction is coinbase
    if (inputs_.size() == 0) {
        return (outputs_.size() == 0);
    }

    // Check if the transaction is well formed aka total of inputs must be equal to total of outputs
    // Calculate the total value of outputs
    int totalOutputValue = 0;
    for (auto& output : outputs_) {
        totalOutputValue += output.getValue();
    }

    // Calculate the total value of inputs
    int totalInputValue = 0;
    // Check if the transaction is valid, aka check if the input is in the UTXO
    for (int i = 0; i < inputs_.size(); i++) {
        bool found = false;
        for (int j = 0; j < utxo.size(); j++) {
            if (inputs_[i].getTxID() == utxo[j].getTransactionId() && inputs_[i].getOutputIndex() == utxo[j].getOutputIndex()) {
                found = true;
                totalInputValue += utxo[j].getOutput().getValue();
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    // Check if the total value of inputs is equal to the total value of outputs
    if (totalInputValue != totalOutputValue) {
        return false;
    }
    // check if all output receivers are in the publicKeys vector
    for (auto& output : outputs_) {
        bool found = false;
        for (auto& key : publicKeys) {
            if (output.getReceiverID() == key) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

void Transaction::fromProtoTransaction(const chat::Transaction& proto_transaction) {
    // read inputs
    for (int i = 0; i < proto_transaction.input_size(); i++) {
        inputs_.push_back(Input(proto_transaction.input(i)));
    }
    // read outputs
    for (int i = 0; i < proto_transaction.output_size(); i++) {
        outputs_.push_back(Output(proto_transaction.output(i)));
    }

    // set all attributes of transaction
    txID_ = proto_transaction.txid();
    senderID_ = static_cast<uint16_t>(proto_transaction.senderid());

    // Get the sendersig string
    std::string sendersig = proto_transaction.sendersig();

    // Allocate an array to hold the sendersig data
    senderSig_ = new unsigned char[sendersig.size()];

    // Copy the sendersig data to the array
    for (int i = 0; i < sendersig.size(); i++) {
        senderSig_[i] = static_cast<unsigned char>(sendersig[i]);
    }

    // Get the public key
    std::string publickey = proto_transaction.publickey();
    for (int i = 0; i < publickey.size() / 2; i++) {
        public_key_.S0[i] = static_cast<unsigned char>(publickey[i]);
        public_key_.S1[i] = static_cast<unsigned char>(publickey[i + publickey.size() / 2]);
    }
}

void Transaction::toProtoTransaction(chat::Transaction* transaction) const {
    for (const auto& input : inputs_) {
        chat::Transaction::Input* proto_input = transaction->add_input();
        proto_input->set_txid(input.getTxID());
        proto_input->set_outputindex(input.getOutputIndex());
    }

    for (const auto& output : outputs_) {
        chat::Transaction::Output* proto_output = transaction->add_output();
        proto_output->set_value(output.getValue());
        proto_output->set_receiverid(output.getReceiverID());
    }

    transaction->set_txid(txID_);
    transaction->set_senderid(static_cast<uint32_t>(senderID_));

    std::string publickey;
    size_t pkSize = sizeof(public_key_.S0);
    for (int i = 0; i < pkSize; i++) {
        publickey += (public_key_.S0[i]);
    }
    for (int i = 0; i < pkSize; i++) {
        publickey += (public_key_.S1[i]);
    }
    transaction->set_publickey(publickey);

    if (senderSig_ == nullptr) {
        throw std::runtime_error("Sender signature is not set.");
    }
    std::string sendersig;
    for (int i = 0; i < senderSigSize_; i++) {
        sendersig += (senderSig_[i]);
    }
    transaction->set_sendersig(sendersig);
}

void Transaction::addInput(Input input) {
    inputs_.push_back(input);
}

void Transaction::addOutput(Output output) {
    outputs_.push_back(output);
}

// hash the concatenation of all the inputs and outputs
std::string Transaction::getDigest() const {
    std::string data = "";
    for (auto& input : inputs_) {
        data += std::to_string(input.getTxID());
        data += std::to_string(input.getOutputIndex());
    }
    for (auto& output : outputs_) {
        data += std::to_string(output.getValue());
        data += output.getReceiverID();
    }
    return signature::hash(data);
}

bool blockchain::operator==(const Transaction& transaction1, const Transaction& transaction2){
        int inputs = 0;
        if (transaction1.inputs_.size() == transaction2.inputs_.size()) {
            for (int i = 0; i < transaction1.inputs_.size(); i++) {
                if (transaction1.inputs_[i] == transaction2.inputs_[i]) {
                    inputs = 1;
                } else {
                    inputs = 0;
                    break;
                }
            }
        }
        int outputs = 0;
        if (transaction1.outputs_.size() == transaction2.outputs_.size()) {
            for (int i = 0; i < transaction1.outputs_.size(); i++) {
                if (transaction1.outputs_[i] == transaction2.outputs_[i]) {
                    outputs = 1;
                } else {
                    outputs = 0;
                    break;
                }
            }
        }

        int pk0 = 0;
        if (sizeof(transaction1.public_key_.S0) == sizeof(transaction2.public_key_.S0)) {
            for (int i = 0; i < sizeof(transaction1.public_key_.S0); i++) {
                if (transaction1.public_key_.S0[i] == transaction2.public_key_.S0[i]) {
                    pk0 = 1;
                } else {
                    pk0 = 0;
                    break;
                }
            }
        }

        int pk1 = 0;
        if (sizeof(transaction1.public_key_.S1) == sizeof(transaction2.public_key_.S1)) {
            for (int i = 0; i < sizeof(transaction1.public_key_.S1); i++) {
                if (transaction1.public_key_.S1[i] == transaction2.public_key_.S1[i]) {
                    pk1 = 1;
                } else {
                    pk1 = 0;
                    break;
                }
            }
        }

        int sig = 0;
        for (int i = 0; i < transaction1.senderSigSize_; i++) {
            if (transaction1.senderSig_[i] == transaction2.senderSig_[i]) {
                sig = 1;
            } else {
                sig = 0;
                break;
            }
        }

        return (transaction1.senderID_ == transaction2.senderID_ && transaction1.txID_ == transaction2.txID_ && inputs && outputs && pk0 && pk1 && sig);
    }

// constructor which sets the txid as its hash but thats wrong
// Transaction::Transaction(std::vector<blockchain::Input> inputs, std::vector<blockchain::Output> outputs,
//         uint16_t senderID, std::string txID, signature::SigKey public_key){
//     for (auto& input : inputs_) {
//         txID_ += input.getTxID();
//         txID_ += std::to_string(input.getOutputIndex());
//     }
//     for (auto& output : outputs_) {
//         txID_ += std::to_string(output.getValue());
//         txID_ += output.getReceiverPK();
//     }
// }