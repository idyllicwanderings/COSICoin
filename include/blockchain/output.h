#ifndef COSICOIN_OUTPUT_H
#define COSICOIN_OUTPUT_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstdint>
#include <string>

#include "json/json.hpp"

using json = nlohmann::json;

namespace blockchain {

class Output {
   public:
    Output(uint64_t value, uint32_t receiverID) : value_(value), receiverID_(receiverID){};
    Output(const chat::Transaction::Output& proto_output);
    Output(){};
    // Copy constructor
    Output(const blockchain::Output& output) : value_(output.getValue()), receiverID_(output.getReceiverID()){};

    uint64_t getValue() const { return value_; };

    uint32_t getReceiverID() const { return receiverID_; };

    friend bool operator==(const Output& output1, const Output& output2) {
        return output1.value_ == output2.value_ && output1.receiverID_ == output2.receiverID_;
    }

    friend bool operator!=(const Output& output1, const Output& output2) {
        return output1.value_ != output2.value_ && output1.receiverID_ != output2.receiverID_;
    }

    friend bool operator<(const Output& output1, const Output& output2) {
        if (output1.value_ == output2.value_) {
            return output1.receiverID_ < output2.receiverID_;
        }
        return output1.value_ < output2.value_;
    }

    friend bool operator>(const Output& output1, const Output& output2) {
        if (output1.value_ == output2.value_) {
            return output1.receiverID_ > output2.receiverID_;
        }
        return output1.value_ > output2.value_;
    }

    /*
     * json converters
     */
    std::string to_string();
    void from_string(std::string output_string);

   private:
    uint64_t value_;
    uint32_t receiverID_;
};
}  // namespace blockchain

#endif