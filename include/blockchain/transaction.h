#ifndef COSICOIN_TRANSACTION_H
#define COSICOIN_TRANSACTION_H

#include <chat.grpc.pb.h>
#include <chat.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstddef>
#include <iomanip>
#include <string>
#include <vector>

#include "input.h"
#include "json/json.hpp"
#include "output.h"
#include "signature/hash.h"
#include "utxo.h"

using json = nlohmann::json;

namespace blockchain {

class Transaction {
   public:
    Transaction(uint32_t txID) : txID_(txID){};
    Transaction(uint32_t txID, uint16_t senderID) : txID_(txID), senderID_(senderID){};
    Transaction(std::vector<blockchain::Input> inputs, std::vector<blockchain::Output> outputs,
                uint16_t senderID, uint32_t txID, signature::SigKey public_key) : inputs_(inputs),
                                                                                  outputs_(outputs),
                                                                                  senderID_(senderID),
                                                                                  txID_(txID),
                                                                                  public_key_(public_key){};
    Transaction() = default;
    // Copy constructor
    Transaction(const blockchain::Transaction& transaction) : inputs_(transaction.getInputs()),
                                                              outputs_(transaction.getOutputs()),
                                                              senderID_(transaction.getSenderID()),
                                                              txID_(transaction.getID()),
                                                              public_key_(transaction.getPublicKey()),
                                                              senderSig_(transaction.getSenderSig()){};

    /*
     * Sets the sender signature
     */
    void setSenderSig(std::vector<std::string> senderSig) { senderSig_ = senderSig; };

    /*
     * Sets the txID
     */
    void settxID(uint32_t txID) { txID_ = txID; };
    /*
     * Sets the sender ID
     */
    void setSenderID(uint16_t senderID) { senderID_ = senderID; };

    /*
     * Returns transaction ID
     */
    uint32_t getID() const { return txID_; };

    /*
     * Returns public key
     */
    signature::SigKey getPublicKey() const { return public_key_; };

    void setPublicKey(signature::SigKey key) { public_key_ = key; };

    /*
     * Returns Sender Signature
     */
    std::vector<std::string> getSenderSig() const { return senderSig_; };

    /*
     * Returns Sender ID
     */
    uint16_t getSenderID() const { return senderID_; };

    /*
     * Verifies if the transaction is valid
     * - Checks if total input value = total output value
     * - Checks if all inputs are in UTXO
     * - Checks if all receivers in ouput are valid receivers
     */
    bool checkSpendingConditions(UTXOlist& utxolist, std::vector<uint32_t>& wallet_ids);

    /*
     * Appends input to list of inputs
     */
    void addInput(Input input);

    /*
     * Appends output to list of outputs
     */
    void addOutput(Output output);

    /*
     * Returns input at given index
     */
    blockchain::Input getInputAt(int index) const { return inputs_[index]; };

    /*
     * Returns vector of inputs
     */
    std::vector<blockchain::Input> getInputs() const { return inputs_; };

    /*
     * Returns output at given index
     */
    blockchain::Output getOutputAt(int index) const { return outputs_[index]; };

    /*
     * Returns vector of outputs
     */
    std::vector<blockchain::Output> getOutputs() const { return outputs_; };

    std::string getDigest() const;

    std::string getStringDigest() const;

    /*
     * Converts itself to a proto buffer transaction
     * Needed to be able to send over GRPC message
     */
    void toProtoTransaction(chat::Transaction* transaction) const;

    /*
     * Converts proto buffer transaction to c++ transaction
     */
    void fromProtoTransaction(const chat::Transaction& proto_transaction);

    friend bool operator==(const Transaction& transaction1, const Transaction& transaction2);

    friend bool operator!=(const Transaction& transaction1, const Transaction& transaction2) {
        return !(transaction1 == transaction2);
    }

    /*
     * json converters
     */
    std::string to_string();
    void from_string(std::string tx_string);

   private:
    std::vector<blockchain::Input> inputs_;
    std::vector<blockchain::Output> outputs_;
    std::vector<std::string> senderSig_;
    uint16_t senderID_;
    uint32_t txID_;
    signature::SigKey public_key_;
};
}  // namespace blockchain

#endif
