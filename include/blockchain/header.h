#ifndef COSICOIN_HEADER_H
#define COSICOIN_HEADER_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <string>
#include <vector>

#include "signature/hash.h"
#include "transaction.h"

namespace blockchain {

class Header {
   public:
    Header(std::string prevBlockDigest) : prevBlockDigest_(prevBlockDigest){};
    Header(const chat::Header& proto_header);
    Header() = default;

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
    chat::Header toProtoHeader() const;

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

   private:
    std::string prevBlockDigest_;
    std::string merkleRoot_;
};

}  // namespace blockchain

#endif