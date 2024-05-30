#ifndef COSICOIN_BLOCK_H
#define COSICOIN_BLOCK_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <ctime>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include "header.h"
#include "input.h"
#include "json/json.hpp"
#include "signature/hash.h"
#include "transaction.h"
#include "utxo.h"

using json = nlohmann::json;

namespace blockchain {

/*
 * Struct to save the signing key of a signature next to the signature
 */
struct BlockSignature {
    std::vector<std::string> signature;
    signature::SigKey public_key;
    uint16_t validator_id;
    uint16_t round;

    friend bool operator==(const BlockSignature& lhs, const BlockSignature& rhs) {
        return lhs.signature == rhs.signature && lhs.public_key == rhs.public_key && lhs.validator_id == rhs.validator_id && lhs.round == rhs.round;
    }
};

class Block {
   public:
    Block(uint64_t id, std::string prevBlockDigest) : header_(blockchain::Header(prevBlockDigest)), id_(id){};
    Block() : empty_(true), mutable_(true), header_(blockchain::Header("")), id_(0){};  // constructor to make empty block
    Block(const chat::Block& proto_block);

    // Copy constructor
    Block(const blockchain::Block& block) : header_(block.getHeader()), txs_(block.getTransactions()), validator_sig_(block.getValidatorSignature()), validator_id_(block.getValidatorID()), validator_next_public_key_(block.getValidatorNextPublicKey()), id_(block.getID()), empty_(block.isEmpty()), mutable_(block.isMutable()), validatorSigs_(block.getValidatorSignatures()){};

    ~Block(){};

    // Getter for ID
    uint64_t getID() const { return id_; };

    // Getter for transactions
    std::vector<Transaction> getTransactions() { return txs_; };
    std::vector<Transaction> getTransactions() const { return txs_; };

    // Getter for digest (hash)
    // Returns null if block is still mutable
    std::string getDigest();
    std::string getDigest() const { return getDigest(); };

    // Getter for header
    // Return null if block is still mutable
    blockchain::Header getHeader() const { return header_; };

    // Needed so that Block can be added to std::set.
    friend bool operator<(const Block& lhs, const Block& rhs);
    friend bool operator>(const Block& lhs, const Block& rhs);
    // Friend function for ==
    friend bool operator==(const Block& lhs, const Block& rhs);
    // Allow streaming of Block on ostreams.
    friend std::ostream& operator<<(std::ostream& o, const Block& b);

    // Returns true if this is empty block
    bool isEmpty() const { return empty_; };

    // Returns true if block is still mutable
    bool isMutable() { return mutable_; };
    bool isMutable() const { return mutable_; };

    /*
     * Converts itself to a proto buffer block
     * Needed to be able to send over GRPC message
     *
     * Important note: block needs to be immutable to be converted!
     */
    inline void toProtoBlock(chat::Block* proto_block) const {
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

    inline void fromProtoBlock(const chat::Block& proto_block) {
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
    }

    /*
     * Appends a transaction
     * - Can only add if block is still mutable
     */
    void addTransaction(blockchain::Transaction tx);

    /*
     * Makes the block immutable
     * - The merkel root gets calculated
     * - After this there can no more transactions be added
     */
    void finalize();

    // Verify block & transactions of block
    bool verify(blockchain::UTXOlist& utxolist, std::vector<uint32_t>& receiverIDs);

    // verify whether the new tx is consistent with the existing txs
    bool verifyTxConsist(const blockchain::Transaction& new_tx) const;

    /*
     * Signs the block with the given signature
     * Calls Signature.Sign() on itself
     * Overwrites the existing signature
     */
    void sign(signature::SigKey signing_key, uint16_t validator_id, signature::SigKey next_public_key);

    bool verifySignature(signature::SigKey public_key);

    std::vector<std::string> getValidatorSignature() { return validator_sig_; };
    std::vector<std::string> getValidatorSignature() const { return validator_sig_; };

    uint16_t getValidatorID() { return validator_id_; };
    uint16_t getValidatorID() const { return validator_id_; };

    signature::SigKey getValidatorNextPublicKey() { return validator_next_public_key_; };
    signature::SigKey getValidatorNextPublicKey() const { return validator_next_public_key_; };

    /*
     * Returns signature for given validator id
     */
    std::vector<BlockSignature> getValidatorSignatures() { return validatorSigs_; };
    std::vector<BlockSignature> getValidatorSignatures() const { return validatorSigs_; };

    /*
     * Adds validator signature to signatures list for given validator id
     */
    void addValidatorSignature(BlockSignature sig) { validatorSigs_.push_back(sig); };

    /*
     * Removes all signatures from block
     */
    void removeSignatures();

    /*
     * json converters
     */
    std::string to_string();
    void from_string(std::string block_string);

   private:
    blockchain::Header header_;
    std::vector<blockchain::Transaction> txs_;
    std::vector<BlockSignature> validatorSigs_;
    std::vector<std::string> validator_sig_;
    uint16_t validator_id_;
    signature::SigKey validator_next_public_key_;
    uint64_t id_;
    bool empty_ = false;
    bool mutable_ = true;
};

}  // namespace blockchain

#endif
