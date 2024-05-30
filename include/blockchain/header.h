#ifndef COSICOIN_HEADER_H
#define COSICOIN_HEADER_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <string>
#include <vector>

#include "json/json.hpp"
#include "signature/hash.h"
#include "transaction.h"

using json = nlohmann::json;

namespace blockchain {

class Header {
   public:
    Header(std::string prevBlockDigest) : prevBlockDigest_(prevBlockDigest){};
    Header(const chat::Header& proto_header) {
        prevBlockDigest_ = proto_header.prevblockdigest();
        merkleRoot_ = proto_header.merkleroot();
    }
    Header() = default;
    // Copy constructor
    Header(const blockchain::Header& header) : prevBlockDigest_(header.getPrevBlockDigest()), merkleRoot_(header.getMerkleRoot()){};

    std::string getID() const;
    std::string getPrevBlockDigest() const { return prevBlockDigest_; }
    std::string getMerkleRoot() const { return merkleRoot_; }

    /*
     * Calculates merkle root of given vector of transactions & stores result
     */
    void calculateMerkleRoot(std::vector<Transaction> transactions);

    /*
     * Converts itself to a proto buffer header
     * Needed to be able to send over GRPC message
     */
    inline chat::Header toProtoHeader() const {
        chat::Header proto_header;

        proto_header.set_prevblockdigest(prevBlockDigest_);
        proto_header.set_merkleroot(merkleRoot_);

        return proto_header;
    }

    /*
     * Overload the equality operator for comparing two Header objects
     */
    friend bool operator==(const Header& lhs, const Header& rhs) {
        return lhs.prevBlockDigest_ == rhs.prevBlockDigest_ && lhs.merkleRoot_ == rhs.merkleRoot_;
    }

    /*
     * Overload the inequality operator for comparing two Header objects
     */
    friend bool operator!=(const Header& lhs, const Header& rhs) {
        return !(lhs == rhs);
    }

    /*
     * json converters
     */
    std::string to_string();
    void from_string(std::string header_string);

   private:
    std::string prevBlockDigest_;
    std::string merkleRoot_;
};

}  // namespace blockchain

#endif
