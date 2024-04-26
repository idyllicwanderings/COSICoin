#ifndef COSICOIN_UTXO_H
#define COSICOIN_UTXO_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <string>

#include "output.h"

namespace blockchain {

class UTXO {
   public:
    UTXO(const uint32_t& transactionId, int outputIndex, const blockchain::Output& output)
        : transactionId_(transactionId), outputIndex_(outputIndex), output_(output){};
    UTXO(const chat::UTXO& proto_utxo)
        : transactionId_(proto_utxo.transaction_id()), outputIndex_(proto_utxo.output_index()), output_(blockchain::Output(proto_utxo.output_value(), proto_utxo.receiver_id())){};

    uint32_t getTransactionId() const { return transactionId_; }
    int getOutputIndex() const { return outputIndex_; }
    blockchain::Output getOutput() const { return output_; }

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

   private:
    uint32_t transactionId_;
    int outputIndex_;
    blockchain::Output output_;
};

}  // namespace blockchain

#endif