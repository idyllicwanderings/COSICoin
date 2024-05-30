#include "blockchain/transaction.h"

using namespace blockchain;

bool Transaction::checkSpendingConditions(UTXOlist& utxolist, std::vector<uint32_t>& wallet_ids) {
    // Check if the transaction is coinbase
    if (inputs_.size() == 0) {
        std::cout << "Transaction::checkSpendingConditions" << " tx is not coinbase" << std::endl;
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
    std::vector<UTXO> utxo = utxolist.getUTXOList();
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
           std::cout << "Transaction::checkSpendingConditions" << " input is not in UTXO" << std::endl;
           return false;
        }
    }
    // Check if the total value of inputs is equal to the total value of outputs
    if (totalInputValue != totalOutputValue) {
        
        std::cout << "Transaction::checkSpendingConditions" << " input is unequal to output" << std::endl;
        return false;
    }
    // check if all output receivers are in the publicKeys vector
    for (auto& output : outputs_) {
        bool found = false;
        for (auto& key : wallet_ids) {
            if (output.getReceiverID() == key) {
                found = true;
                break;
            }
        }
        if (!found) {
            
            std::cout << "Transaction::checkSpendingConditions" << "output's ids is not among the sets" << std::endl;
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

    // read sendersig
    senderSig_.clear();
    for (int i = 0; i < proto_transaction.sendersig_size(); i++) {
        senderSig_.push_back(proto_transaction.sendersig(i));
    }

    // Get the public key
    std::string publickey = proto_transaction.publickey();
    if (!publickey.empty()) {
        public_key_ = signature::SigKey_from_string(publickey);
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
    if (!public_key_.S0.empty() && !public_key_.S1.empty()) {
        publickey = signature::SigKey_to_string(public_key_);
    }
    transaction->set_publickey(publickey);

    for (const std::string ss : senderSig_) {
        transaction->add_sendersig(ss);
    }
}

void Transaction::addInput(Input input) {
    inputs_.push_back(input);
}

void Transaction::addOutput(Output output) {
    outputs_.push_back(output);
}

// hash the concatenation of all the inputs and outputs
std::string Transaction::getDigest() const {
    std::string data = std::to_string(txID_);
    for (auto& input : inputs_) {
        data += std::to_string(input.getTxID());
        data += std::to_string(input.getOutputIndex());
    }
    for (auto& output : outputs_) {
        data += std::to_string(output.getValue());
        data += std::to_string(output.getReceiverID());
    }
    return signature::hash(data);
}

std::string Transaction::getStringDigest() const {
    std::string data = std::to_string(txID_);
    for (auto& input : inputs_) {
        data += std::to_string(input.getTxID());
        data += std::to_string(input.getOutputIndex());
    }
    for (auto& output : outputs_) {
        data += std::to_string(output.getValue());
        data += std::to_string(output.getReceiverID());
    }
    return data;
}

bool blockchain::operator==(const Transaction& transaction1, const Transaction& transaction2) {
    // Check inputs
    if (transaction1.getInputs().size() != transaction2.getInputs().size()) {
        return false;
    }
    for (int i = 0; i < transaction1.getInputs().size(); i++) {
        if (transaction1.getInputAt(i) != transaction2.getInputAt(i)) {
            return false;
        }
    }

    // Check outputs
    if (transaction1.getOutputs().size() != transaction2.getOutputs().size()) {
        return false;
    }
    for (int i = 0; i < transaction1.getOutputs().size(); i++) {
        if (transaction1.getOutputAt(i) != transaction2.getOutputAt(i)) {
            return false;
        }
    }

    // Check sendersig
    if (transaction1.getSenderSig() != transaction2.getSenderSig()) {
        return false;
    }

    // Check sender id
    if (transaction1.getSenderID() != transaction2.getSenderID()) {
        return false;
    }

    // Check txid
    if (transaction1.getID() != transaction2.getID()) {
        return false;
    }

    // Check public key
    if (transaction1.getPublicKey() != transaction2.getPublicKey()) {
        return false;
    }

    return true;
}

std::string Transaction::to_string() {
    json j;

    j["txID"] = txID_;

    for (blockchain::Input input : inputs_) {
        j["inputs"].push_back(input.to_string());
    }

    for (blockchain::Output output : outputs_) {
        j["outputs"].push_back(output.to_string());
    }

    j["senderID"] = senderID_;

    j["senderSig"] = signature::signature_to_json(senderSig_);

    if (!public_key_.S0.empty() && !public_key_.S1.empty()) {
        j["publicKey"] = signature::SigKey_to_string(public_key_);
    }

    return j.dump();
}

void Transaction::from_string(std::string tx_string) {
    json j = json::parse(tx_string);

    txID_ = j.at("txID");

    inputs_.clear();
    for (std::string el : j.at("inputs")) {
        blockchain::Input input;
        input.from_string(el);
        inputs_.push_back(input);
    }

    outputs_.clear();
    for (std::string el : j.at("outputs")) {
        blockchain::Output output;
        output.from_string(el);
        outputs_.push_back(output);
    }

    senderID_ = j.at("senderID");

    senderSig_ = signature::json_to_string(j.at("senderSig"));

    if (j.contains("publicKey")) {
        public_key_ = signature::SigKey_from_string(j.at("publicKey"));
    }
}
