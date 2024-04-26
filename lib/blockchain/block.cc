#include "blockchain/block.h"

#define KEY_LEN_ 256

using namespace blockchain;

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
    proto_block->set_validatorsig(validator_sig_);

    // Convert validator id
    proto_block->set_validatorid(static_cast<uint32_t>(validator_id_));

    // Convert next public key
    std::string publickey;
    size_t pkSize = sizeof(validator_next_public_key_.S0);
    for (int i = 0; i < pkSize; i++) {
        publickey += (validator_next_public_key_.S0[i]);
    }
    for (int i = 0; i < pkSize; i++) {
        publickey += (validator_next_public_key_.S1[i]);
    }
    proto_block->set_publickey(publickey);

    // Convert block ID
    proto_block->set_blockid(id_);
}

Block::Block(const chat::Block& proto_block) : header_(proto_block.header()), id_(proto_block.blockid()) {
    // Read transactions
    for (int i = 0; i < proto_block.transaction_size(); i++) {
        blockchain::Transaction tx;
        tx.fromProtoTransaction(proto_block.transaction(i));
        txs_.push_back(tx);
    }

    // Read validator signature
    validator_sig_ = proto_block.validatorsig();

    // Read validator id
    validator_id_ = static_cast<uint16_t>(proto_block.validatorid());

    // Read next pub key
    std::string publickey = proto_block.publickey();
    int pk_size = publickey.size() / 2;
    for (int i = 0; i < pk_size; i++) {
        validator_next_public_key_.S0[i] = static_cast<unsigned char>(publickey[i]);
        validator_next_public_key_.S1[i] = static_cast<unsigned char>(publickey[i + pk_size]);
    }

    mutable_ = false;
    empty_ = false;
}

void Block::fromProtoBlock(const chat::Block& proto_block) {
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
    validator_sig_ = proto_block.validatorsig();

    // Read validator id
    validator_id_ = static_cast<uint16_t>(proto_block.validatorid());

    // Read next pub key
    std::string publickey = proto_block.publickey();
    int pk_size = publickey.size() / 2;
    for (int i = 0; i < pk_size; i++) {
        validator_next_public_key_.S0[i] = static_cast<unsigned char>(publickey[i]);
        validator_next_public_key_.S1[i] = static_cast<unsigned char>(publickey[i + pk_size]);
    }

    mutable_ = false;
    empty_ = false;
}

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

bool Block::verify(std::vector<UTXO>& utxo, std::vector<uint32_t>& receiverIDs) {
    // Verify every transaction
    std::set<blockchain::Input> all_inputs;
    int number_of_inputs = 0;
    for (auto& transaction : txs_) {
        if (!transaction.checkSpendingConditions(utxo, receiverIDs)) {
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

    return true;
}

std::ostream& blockchain::operator<<(std::ostream& o, const Block& b) {
    o << "block_id: " << b.getID() << std::endl;
    return o;
}

std::string Block::charToString_(const unsigned char* charlist, int size) const {
    std::string string;
    for (int i = 0; i < size; i++) {
        string += (charlist[i]);
    }
    return string;
}

void Block::stringToChar_(unsigned char* charlist, std::string string) {
    for (int i = 0; i < string.size(); i++) {
        charlist[i] = static_cast<unsigned char>(string[i]);
    }
}

void Block::sign(signature::Signature current_keypair, uint16_t validator_id, signature::SigKey next_public_key) {
    if (mutable_) {
        throw std::runtime_error("Cannot sign mutable block.");
    }
    // Get digest to sign
    std::string digest_str = getDigest();
    unsigned char digest_chr[digest_str.size()];
    stringToChar_(digest_chr, digest_str);
    // Sign digest
    unsigned char signature[KEY_LEN_];
    current_keypair.Sign(digest_chr[0], signature);  // TODO: sign entire digest instead of only first char
    validator_sig_ = charToString_(signature, KEY_LEN_);
    // Save id & key
    validator_id_ = validator_id;
    validator_next_public_key_ = next_public_key;
}

int Block::getValidatorSignatureFromList(unsigned char* signature_chr, uint16_t validator_id) {
    if (validatorSigs_.count(validator_id) == 1) {
        // Return signature for given id
        std::string signature_str = validatorSigs_[validator_id];
        stringToChar_(signature_chr, signature_str);
        return 1;
    }
    return 0;
}

void Block::addValidatorSignature(uint16_t validator_id, unsigned char* signature) {
    validatorSigs_[validator_id] = charToString_(signature, KEY_LEN_);
}

void Block::getValidatorSignature(unsigned char* signature) {
    stringToChar_(signature, validator_sig_);
}
