#ifndef COSICOIN_INPUT_H
#define COSICOIN_INPUT_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <string>

namespace blockchain {

class Input {
   public:
    Input(uint32_t txID, uint64_t outputIndex) : txID_(txID), outputIndex_(outputIndex){};
    Input(const chat::Transaction::Input& proto_input);

    uint32_t getTxID() const { return txID_; };

    uint64_t getOutputIndex() const { return outputIndex_; };

    /*
     * Converts itself to a proto buffer input
     * Needed to be able to send over GRPC message
     */
    // chat::Input toProtoInput() const;

    friend bool operator==(const Input& input1, const Input& input2) {
        return input1.txID_ == input2.txID_ && input1.outputIndex_ == input2.outputIndex_;
    }

    friend bool operator<(const Input& input1, const Input& input2) {
        if (input1.txID_ < input2.txID_) {
            return true;
        } else if (input1.txID_ == input2.txID_) {
            return input1.outputIndex_ < input2.outputIndex_;
        } else {
            return false;
        }
    }

    friend bool operator>(const Input& input1, const Input& input2) {
        if (input1.txID_ > input2.txID_) {
            return true;
        } else if (input1.txID_ == input2.txID_) {
            return input1.outputIndex_ > input2.outputIndex_;
        } else {
            return false;
        }
    }

   private:
    uint32_t txID_;
    uint64_t outputIndex_;
};

}  // namespace blockchain
#endif