#include "blockchain/block.h"

#define KEY_LEN_ 256

using namespace blockchain;

/*
void Block::toProtoBlock(chat::Block* proto_block) const {
    if (mutable_) {
        throw std::runtime_error("Cannot convert mutable block.");
    }

    // Convert header
    chat::Header proto_header = header_.toProtoHeader();
    *proto_block->mutable_header() = proto_header;

    // Convert transactions
    for (int i = 0; i < txs_.size(); i++) {
        txs_[i].toProtoTransaction(proto_block->add_transaction());
    }

    // Convert validator signature
    for (const std::string sig : validator_sig_) {
        proto_block->add_validatorsig(sig);
    }

    // Convert validator id
    proto_block->set_validatorid(static_cast<uint32_t>(validator_id_));

    // Convert next public key
    std::string publickey = signature::SigKey_to_string(validator_next_public_key_);
    proto_block->set_publickey(publickey);

    // Convert block ID
    proto_block->set_blockid(id_);
}
*/

Block::Block(const chat::Block& proto_block) : header_(proto_block.header()), id_(proto_block.blockid()) {
    // Read transactions
    for (int i = 0; i < proto_block.transaction_size(); i++) {
        blockchain::Transaction tx;
        tx.fromProtoTransaction(proto_block.transaction(i));
        txs_.push_back(tx);
    }

    // Read validator signature
    validator_sig_.clear();
    for (int i = 0; i < proto_block.validatorsig_size(); i++) {
        validator_sig_.push_back(proto_block.validatorsig(i));
    }

    // Read validator id
    validator_id_ = static_cast<uint16_t>(proto_block.validatorid());

    // Read next pub key
    std::string publickey = proto_block.publickey();
    validator_next_public_key_ = signature::SigKey_from_string(publickey);

    mutable_ = false;
    empty_ = false;
}

/* void Block::fromProtoBlock(const chat::Block& proto_block) {
    // Read header
    header_ = proto_block.header();

    // Read id
    id_ = proto_block.blockid();

    // Read transactions
    for (int i = 0; i < proto_block.transaction_size(); i++) {
        blockchain::Transaction tx;
        tx.fromProtoTransaction(proto_block.transaction(i));
        txs_.push_back(tx);
    }

    // Read validator signature
    validator_sig_.clear();
    for (int i = 0; i < proto_block.validatorsig_size(); i++) {
        validator_sig_.push_back(proto_block.validatorsig(i));
    }

    // Read validator id
    validator_id_ = static_cast<uint16_t>(proto_block.validatorid());

    // Read next pub key
    std::string publickey = proto_block.publickey();
    validator_next_public_key_ = signature::SigKey_from_string(publickey);

    mutable_ = false;
    empty_ = false;
}*/

void Block::addTransaction(Transaction tx) {
    if (mutable_) {
        txs_.push_back(tx);
    } else {
        throw std::runtime_error("Cannot add transaction to an immutable block.");
    }
}

// Make block immutable
void Block::finalize() {
    if (!mutable_) {
        throw std::runtime_error("Cannot finalize immutable block.");
    }
    mutable_ = false;
    header_.calculateMerkleRoot(txs_);
}

bool Block::verify(blockchain::UTXOlist& utxolist, std::vector<uint32_t>& receiverIDs) {
    // Verify every transaction
    std::set<blockchain::Input> all_inputs;
    std::vector<blockchain::UTXO> utxo = utxolist.getUTXOList();
    int number_of_inputs = 0;
    for (auto& transaction : txs_) {
        if (!transaction.checkSpendingConditions(utxolist, receiverIDs)) {
            // At least one transaction is not valid
            return false;
        }
        // add inputs to set of all inputs
        const std::vector<blockchain::Input> inputs = transaction.getInputs();
        number_of_inputs += inputs.size();
        all_inputs.insert(inputs.begin(), inputs.end());
    }

    // Check that no two inputs are the same
    if (number_of_inputs != all_inputs.size()) {
        return false;
    }

    return true;
}

bool Block::verifyTxConsist(const blockchain::Transaction& new_tx) const {
    // Check that no input already occurs in added transactions
    std::vector<blockchain::Input> new_inputs = new_tx.getInputs();
    for (int i = 0; i < txs_.size(); i++) {
        for (blockchain::Input input : txs_[i].getInputs()) {
            if (std::find(new_inputs.begin(), new_inputs.end(), input) != new_inputs.end()) {
                return false;
            }
        }
    }
    return true;
}

bool operator<(const Block& lhs, const Block& rhs) {
    if (lhs.isEmpty() && rhs.isEmpty()) {
        return false;
    }
    return lhs.getID() < rhs.getID();
}

bool operator>(const Block& lhs, const Block& rhs) {
    if (lhs.isEmpty() && rhs.isEmpty()) {
        return false;
    }
    return lhs.getID() > rhs.getID();
}

bool blockchain::operator==(const Block& lhs, const Block& rhs) {
    // Check if the blocks have the same ID
    if (lhs.getID() != rhs.getID()) {
        return false;
    }

    // Check if the headers are equal
    if (lhs.getHeader() != rhs.getHeader()) {
        return false;
    }

    // Check if the blocks have the same number of transactions
    if (const_cast<Block&>(lhs).getTransactions().size() != const_cast<Block&>(rhs).getTransactions().size()) {
        return false;
    }

    // Check if the transactions in the blocks are equal
    for (int i = 0; i < const_cast<Block&>(lhs).getTransactions().size(); i++) {
        if (const_cast<Block&>(lhs).getTransactions()[i] != const_cast<Block&>(rhs).getTransactions()[i]) {
            return false;
        }
    }

    // Check if validator signature is equal
    if (lhs.validator_sig_ != rhs.validator_sig_) {
        return false;
    }

    // Check if validator id is equal
    if (lhs.validator_id_ != rhs.validator_id_) {
        return false;
    }

    // Check if next public key is equal
    if (lhs.validator_next_public_key_ != rhs.validator_next_public_key_) {
        return false;
    }

    // Check of validator signatures are equal
    if (lhs.validatorSigs_.size() != rhs.validatorSigs_.size()) {
        return false;
    }

    for (int i = 0; i < lhs.validatorSigs_.size(); i++) {
        if (lhs.validatorSigs_[i].validator_id != rhs.validatorSigs_[i].validator_id) {
            return false;
        }
        if (lhs.validatorSigs_[i].signature != rhs.validatorSigs_[i].signature) {
            return false;
        }
        if (lhs.validatorSigs_[i].round != rhs.validatorSigs_[i].round) {
            return false;
        }
        if (lhs.validatorSigs_[i].public_key != rhs.validatorSigs_[i].public_key) {
            return false;
        }
    }

    return true;
}

std::ostream& blockchain::operator<<(std::ostream& o, const Block& b) {
    o << "block_id: " << b.getID() << std::endl;
    return o;
}

void Block::sign(signature::SigKey signing_key, uint16_t validator_id, signature::SigKey next_public_key) {
    if (mutable_) {
        throw std::runtime_error("Cannot sign mutable block.");
    }
    // Save id & key
    validator_id_ = validator_id;
    validator_next_public_key_ = next_public_key;
    // Get digest to sign
    std::string digest_str = getDigest();
    // Sign digest
    validator_sig_.clear();
    validator_sig_ = signature::Sign(digest_str, signing_key);
}

std::string Block::to_string() {
    json j;

    j["header"] = header_.to_string();

    j["blockID"] = id_;

    for (blockchain::Transaction tx : txs_) {
        j["transactions"].push_back(tx.to_string());
    }

    j["validatorSig"] = signature::signature_to_json(validator_sig_);

    j["validatorID"] = validator_id_;

    j["publicKey"] = signature::SigKey_to_string(validator_next_public_key_);

    std::cout << "validatorSigs size: " << validatorSigs_.size() << std::endl;
    for (BlockSignature sig : validatorSigs_) {
        std::cout << "validatorSigs: " << sig.validator_id << std::endl;
        j["validatorSigs"].push_back({{"validator_id", sig.validator_id},
                                      {"signature", signature::signature_to_json(sig.signature)},
                                      {"round", sig.round},
                                      {"public_key", signature::SigKey_to_string(sig.public_key)}});
    }

    return j.dump();
}

void Block::from_string(std::string block_string) {
    json j = json::parse(block_string);

    header_.from_string(j.at("header"));
    id_ = j.at("blockID");
    txs_.clear();
    for (std::string el : j.at("transactions")) {
        blockchain::Transaction tx;
        tx.from_string(el);
        txs_.push_back(tx);
    }
    validator_sig_ = signature::json_to_string(j.at("validatorSig"));
    validator_id_ = j.at("validatorID");
    validator_next_public_key_ = signature::SigKey_from_string(j.at("publicKey"));

    validatorSigs_.clear();
    if (j.contains("validatorSigs")) {
        for (const auto& el : j.at("validatorSigs")) {
            BlockSignature sig;
            sig.validator_id = el.at("validator_id");
            sig.signature = signature::json_to_string(el.at("signature"));
            sig.round = el.at("round");
            sig.public_key = signature::SigKey_from_string(el.at("public_key"));
            validatorSigs_.push_back(sig);
        }
    }

    empty_ = false;
    mutable_ = false;
}

std::string Block::getDigest() {
    if (mutable_) {
        throw std::runtime_error("Cannot digest mutable block.");
    }
    std::string input_str = header_.getID();
    input_str += std::to_string(validator_id_);
    if (!validator_next_public_key_.S0.empty() && !validator_next_public_key_.S1.empty()) {
        input_str += signature::SigKey_to_string(validator_next_public_key_);
    }
    return signature::hash(input_str);
}

bool Block::verifySignature(signature::SigKey public_key) {
    std::string digest_str = getDigest();
    return signature::Verify(digest_str, validator_sig_, public_key) == 1;
}

void Block::removeSignatures() {
    validator_sig_.clear();
    validator_id_ = 0;
    validator_next_public_key_ = signature::SigKey();
    validatorSigs_.clear();
}
