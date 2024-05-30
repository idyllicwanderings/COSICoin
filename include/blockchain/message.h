#ifndef COSICOIN_INCLUDE_MESSAGE_H
#define COSICOIN_INCLUDE_MESSAGE_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "block.h"
#include "transaction.h"
#include "utxo.h"

namespace blockchain {

enum MsgType { ECHO,
               SEND,
               READY };

class Message {
   public:
    /*
     * Constructor to make empty message
     */
    Message() : empty_(true), block_(Block()){};
    /*
     * Basic constructor
     */
    Message(MsgType type, const blockchain::Block& block, uint64_t sender_id, uint16_t round)
        : type_(type), block_(block), sender_id_(sender_id), round_(round), empty_(false) {}
    /*
     * Constructor to make message from proto message
     */
    Message(const chat::Message& proto_message) {  // Don't move this implementation to message.cc because otherwise the ghosts in the ESAT computers will hide it from the compiler when linking it to an executable
        uint32_t proto_type = proto_message.type();
        if (proto_type == 0) {
            type_ = MsgType::ECHO;
        } else if (proto_type == 1) {
            type_ = MsgType::SEND;
        } else if (proto_type == 2) {
            type_ = MsgType::READY;
        }
        sender_id_ = proto_message.sender_id();
        round_ = proto_message.round();
        empty_ = false;
        block_.fromProtoBlock(proto_message.block());
    };
    //~Message();

    MsgType getType() const { return type_; };

    int setType(const MsgType type_message);

    const blockchain::Block& getBlock() const { return block_; };

    int getSenderId() const { return sender_id_; };

    int getRound() const { return round_; };

    bool isEmpty() const { return empty_; };

    friend std::ostream& operator<<(std::ostream& o, const Message& m);

    friend std::string Msgtype2String(MsgType type);

    /*
     * Converts message to proto message
     */
    chat::Message toProtoMessage() const {  // Same as for the constructor implementation
        chat::Message proto_message;

        if (type_ == MsgType::ECHO) {
            proto_message.set_type(0);
        } else if (type_ == MsgType::SEND) {
            proto_message.set_type(1);
        } else if (type_ == MsgType::READY) {
            proto_message.set_type(2);
        }

        chat::Block* proto_block = proto_message.mutable_block();
        block_.toProtoBlock(proto_block);

        proto_message.set_sender_id(sender_id_);
        proto_message.set_round(round_);

        return proto_message;
    };

   private:
    MsgType type_;
    blockchain::Block block_;
    uint64_t sender_id_;
    uint32_t round_;
    bool empty_;
};

}  // namespace blockchain
#endif
