#ifndef COSICOIN_TRANSACTION_H
#define COSICOIN_TRANSACTION_H

#include <chat.grpc.pb.h>
#include <chat.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstddef>
#include <string>
#include <vector>

#include "input.h"
#include "output.h"
#include "signature/hash.h"
#include "utxo.h"

namespace blockchain {

class Transaction {
   public:
    Transaction(uint32_t txID) : txID_(txID){};
    // Transaction(const chat::Transaction &proto_transaction);
    // Transaction(std::vector<blockchain::Input> inputs, std::vector<blockchain::Output> outputs,
    //     unsigned char* senderSig, uint16_t senderID, std::string txID, signature::SigKey public_key) : inputs_(inputs),
    //     outputs_(outputs), senderSig_(senderSig), senderID_(senderID), txID_(txID), public_key_(public_key){};
    Transaction(std::vector<blockchain::Input> inputs, std::vector<blockchain::Output> outputs,
                uint16_t senderID, uint32_t txID, signature::SigKey public_key) : inputs_(inputs),
                                                                                  outputs_(outputs),
                                                                                  senderID_(senderID),
                                                                                  txID_(txID),
                                                                                  public_key_(public_key){};
    Transaction() = default;

    /*
     * Sets the sender signature
     */
    void setSenderSig(unsigned char* senderSig) { senderSig_ = senderSig; };

    /*
     * Sets the txID
     */
    void settxID(uint32_t txID) { txID_ = txID; };

    /*
     * Returns transaction ID
     */
    uint32_t getID() const { return txID_; };

    /*
     * Returns public key
     */
    signature::SigKey getPublicKey() const { return public_key_; };

    /*
     * Returns Sender Signature
     */
    unsigned char* getSenderSig() const { return senderSig_; };

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
    bool checkSpendingConditions(std::vector<UTXO>& utxo, std::vector<uint32_t>& publicKeys);

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

   private:
    std::vector<blockchain::Input> inputs_;
    std::vector<blockchain::Output> outputs_;
    unsigned char* senderSig_ = nullptr;
    uint16_t senderID_;
    uint32_t txID_;
    signature::SigKey public_key_;
    uint16_t senderSigSize_ = 4;
};
}  // namespace blockchain

#endif