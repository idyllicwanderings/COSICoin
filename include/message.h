#ifndef COSICOIN_INCLUDE_MESSAGE_H
#define COSICOIN_INCLUDE_MESSAGE_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "block.h"

namespace blockchain {

enum MsgType { ECHO,
               SEND,
               READY };



class Message {
   public:
    Message(MsgType type, const blockchain::Block& block, uint64_t sender_id, uint16_t round) : type_(type), block_(block), sender_id_(sender_id), round_(round), empty_(false) {}
    Message(const chat::Message& proto_message);
    Message() : empty_(true), block_(Block()){};
    //~Message();

    MsgType getType() const { return this->type_; };

    int setType(const MsgType type_message);

    const blockchain::Block& getBlock() const { return this->block_; };

    int getSenderId() const { return this->sender_id_; };

    int getRound() const { return round_; };

    bool isEmpty() const { return empty_; };

    friend std::ostream& operator<<(std::ostream& o, const Message& m);
    
    friend std::string Msgtype2String(MsgType type);

    chat::Message toProtoMessage() const;

   private:
    MsgType type_;
    blockchain::Block block_;
    uint64_t sender_id_;
    uint32_t round_;
    bool empty_;
};

}  // namespace blockchain
#endif
