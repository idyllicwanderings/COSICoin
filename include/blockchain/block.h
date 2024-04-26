#ifndef COSICOIN_BLOCK_H
#define COSICOIN_BLOCK_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <ctime>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include "header.h"
#include "input.h"
#include "signature/hash.h"
#include "transaction.h"
#include "utxo.h"

namespace blockchain {

class Block {
   public:
    Block(uint64_t id, std::string prevBlockDigest) : header_(blockchain::Header(prevBlockDigest)), id_(id){};
    Block() : empty_(true), mutable_(false), header_(blockchain::Header("")), id_(0){};  // constructor to make empty block
    Block(const chat::Block& proto_block);
    // Block() = default;
    ~Block(){};

    // Getter for ID
    uint64_t getID() const { return id_; };

    // Getter for transactions
    std::vector<Transaction> getTransactions() { return txs_; };

    // Getter for digest (hash)
    // Returns null if block is still mutable
    std::string getDigest() const { return header_.getID(); };

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

    /*
     * Converts itself to a proto buffer block
     * Needed to be able to send over GRPC message
     *
     * Important note: block needs to be immutable to be converted!
     */
    void toProtoBlock(chat::Block* proto_block) const;

    void fromProtoBlock(const chat::Block& proto_block);

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
    bool verify(std::vector<UTXO>& utxo, std::vector<uint32_t>& receiverIDs);

    // verify whether the new tx is consistent with the existing txs
    bool verifyTxConsist(const blockchain::Transaction& new_tx) const;

    /*
     * Signs the block with the given signature
     * Calls Signature.Sign() on itself
     * Overwrites the existing signature
     */
    void sign(signature::Signature current_keypair, uint16_t validator_id, signature::SigKey next_public_key);

    /*
     * Writes validator signature to given location
     * Make sure given location has enough space allocated (256)
     */
    void getValidatorSignature(unsigned char* signature);

    uint16_t getValidatorID() { return validator_id_; };

    signature::SigKey getValidatorNextPublicKey() { return validator_next_public_key_; };

    /*
     * Returns signature for given validator id
     * Returns 0 if no signature for given validator id
     */
    int getValidatorSignatureFromList(unsigned char* signature_chr, uint16_t validator_id);

    /*
     * Adds validator signature to signatures list for given validator id
     */
    void addValidatorSignature(uint16_t validator_id, unsigned char* signature);

    std::string charToString_(const unsigned char* charlist, int size) const;
    void stringToChar_(unsigned char* charlist, std::string string);

   private:
    blockchain::Header header_;
    std::vector<blockchain::Transaction> txs_;
    std::map<uint16_t, std::string> validatorSigs_;
    std::string validator_sig_;
    uint16_t validator_id_;
    signature::SigKey validator_next_public_key_;
    uint64_t id_;
    bool empty_ = false;
    bool mutable_ = true;
};

}  // namespace blockchain

#endif
