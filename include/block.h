#ifndef COSICOIN_BLOCK_H
#define COSICOIN_BLOCK_H

#include "transaction.h"
// #include "message.h"

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstdint>
#include <ctime>
#include <sstream>
#include <string>

// using namespace message;
// using namespace blockchain;

namespace blockchain {

class Block {
   public:
    Block(uint64_t id) : id_(id), empty_(false), hash_(std::to_string(id)){};
    Block(uint64_t id, std::string transaction) : id_(id), transactions_(transaction), empty_(false){};
    Block() : empty_(true){};
    Block(const chat::Block& proto_block);  // Constructor from proto block
    ~Block(){};

    uint64_t getId() const { return this->id_; }

    std::time_t getTime() const { return this->time_; }

    std::string getTransaction() const { return this->transactions_; }

    // Calculate the hash of the Block.
    std::string calculateHash() const;

    // Needed so that Block can be added to std::set.
    friend bool operator<(const Block& lhs, const Block& rhs);

    // Allow streaming of Block on ostreams.
    friend std::ostream& operator<<(std::ostream& o, const Block& b);

    std::string getHash() const { return this->hash_; }

    chat::Block toProtoBlock() const;

   private:
    std::time_t time_;
    std::string transactions_;
    uint64_t id_;
    std::string prev_hash_;
    std::string hash_;
    uint64_t nonce_;
    bool empty_;
};

}  // namespace blockchain

#endif
