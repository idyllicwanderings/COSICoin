#ifndef COSICOIN_UTXO_H
#define COSICOIN_UTXO_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <string>
#include <vector>

#include "input.h"
#include "output.h"

namespace blockchain {

class UTXO {
   public:
    UTXO(const uint32_t& transactionId, int outputIndex, const blockchain::Output& output)
        : transactionId_(transactionId), outputIndex_(outputIndex), output_(output){};
    UTXO(const chat::UTXO& proto_utxo)
        : transactionId_(proto_utxo.transaction_id()), outputIndex_(proto_utxo.output_index()), output_(blockchain::Output(proto_utxo.output_value(), proto_utxo.receiver_id())){};
    // Copy constructor
    UTXO(const blockchain::UTXO& utxo) : transactionId_(utxo.getTransactionId()), outputIndex_(utxo.getOutputIndex()), output_(utxo.getOutput()){};

    uint32_t getTransactionId() const { return transactionId_; }
    int getOutputIndex() const { return outputIndex_; }
    blockchain::Output getOutput() const { return output_; }

    /*
     * Converts to proto UTXO
     */
    void toProtoUTXO(chat::UTXO* proto_utxo) const {
        proto_utxo->set_transaction_id(transactionId_);
        proto_utxo->set_output_index(outputIndex_);
        proto_utxo->set_output_value(output_.getValue());
        proto_utxo->set_receiver_id(output_.getReceiverID());
    }

    friend bool operator==(const UTXO& lhs, const UTXO& rhs) {
        return lhs.transactionId_ == rhs.transactionId_ && lhs.outputIndex_ == rhs.outputIndex_ &&
               lhs.output_ == rhs.output_;
    }

    friend bool operator>(const UTXO& lhs, const UTXO& rhs) {
        if (lhs.transactionId_ == rhs.transactionId_) {
            return lhs.outputIndex_ > rhs.outputIndex_;
        } else {
            return lhs.transactionId_ > rhs.transactionId_;
        }
    }

    friend bool operator<(const UTXO& lhs, const UTXO& rhs) {
        if (lhs.transactionId_ == rhs.transactionId_) {
            return lhs.outputIndex_ < rhs.outputIndex_;
        } else {
            return lhs.transactionId_ < rhs.transactionId_;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const UTXO& utxo) {
        os << "UTXO: Transaction ID: " << utxo.transactionId_ << ", "
           << "Output Index: " << utxo.outputIndex_ << ", "
           << "Output Value: " << utxo.output_.getValue() << ", "
           << "Receiver ID: " << utxo.output_.getReceiverID();
        return os;
    }

   private:
    uint32_t transactionId_;
    int outputIndex_;
    blockchain::Output output_;
};

/*
 * Wrapper class for vector<UTXO>
 */
class UTXOlist {
   public:
    UTXOlist(){};
    UTXOlist(std::vector<UTXO>& utxolist) : utxolist_(utxolist) {}
    UTXOlist(chat::UTXOlist& proto_utxolist) {
        for (int i = 0; i < proto_utxolist.utxo_size(); i++) {
            blockchain::UTXO utxo(proto_utxolist.utxo(i));
            utxolist_.push_back(utxo);
        }
    }
    // Copy constructor
    UTXOlist(const blockchain::UTXOlist& utxolist) : utxolist_(utxolist.getUTXOList()) {}

    auto begin() { return utxolist_.begin(); }
    auto end() { return utxolist_.end(); }

    inline void add(const UTXO& utxo) {
        utxolist_.push_back(utxo);
    }

    std::vector<UTXO>& getUTXOList() { return utxolist_; }
    const std::vector<UTXO>& getUTXOList() const { return utxolist_; }

    void clear() {
        utxolist_.clear();
    }

    int size() {
        return utxolist_.size();
    }

    UTXO operator[](int index) {
        return utxolist_[index];
    }

    bool empty() {
        return utxolist_.empty();
    }

    int index(Input& input) {
        for (int i = 0; i < utxolist_.size(); i++) {
            if (utxolist_[i].getTransactionId() == input.getTxID() &&
                utxolist_[i].getOutputIndex() == input.getOutputIndex()) {
                return i;
            }
        }
        return -1;
    }

    void erase(int index) {
        utxolist_.erase(utxolist_.begin() + index);
    }

    /*
     * Converts to proto UTXOlist
     */
    void toProtoUTXOlist(chat::UTXOlist* proto_utxolist) {
        for (int i = 0; i < utxolist_.size(); i++) {
            utxolist_[i].toProtoUTXO(proto_utxolist->add_utxo());
        }
    }

    /*
     * Converts from proto UTXOlist
     */
    void fromProtoUTXOlist(const chat::UTXOlist& proto_utxolist) {
        for (int i = 0; i < proto_utxolist.utxo_size(); i++) {
            blockchain::UTXO utxo(proto_utxolist.utxo(i));
            utxolist_.push_back(utxo);
        }
    }

    friend bool operator==(const UTXOlist& lhs, const UTXOlist& rhs) {
        return lhs.utxolist_ == rhs.utxolist_;
    }

    friend std::ostream& operator<<(std::ostream& os, const UTXOlist& utxolist) {
        os << "UTXOlist: ";
        for (const auto& utxo : utxolist.utxolist_) {
            os << utxo << ", ";
        }
        return os;
    }

   private:
    std::vector<UTXO> utxolist_;
};

}  // namespace blockchain

#endif