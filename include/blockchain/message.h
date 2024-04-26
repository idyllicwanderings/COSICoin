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

enum MsgContent { BLOCK,
                  TRANSACTION,
                  UTXOLIST };

class Message {
   public:
    Message(MsgType type, const blockchain::Block& block, uint64_t sender_id, uint16_t round)
        : type_(type), block_(block), sender_id_(sender_id), round_(round), empty_(false), content_type_(BLOCK) {}
    Message(MsgType type, const blockchain::Transaction& transaction, uint64_t sender_id, uint16_t round)
        : type_(type), transaction_(transaction), sender_id_(sender_id), round_(round), empty_(false), content_type_(TRANSACTION) {}
    Message(MsgType type, std::vector<blockchain::UTXO>& utxo, uint64_t sender_id, uint16_t round)
        : type_(type), utxolist_(utxo), sender_id_(sender_id), round_(round), empty_(false), content_type_(UTXOLIST) {}
    Message(const chat::Message& proto_message);
    Message() : empty_(true), block_(Block()){};
    //~Message();

    MsgType getType() const { return type_; };

    int setType(const MsgType type_message);

    const blockchain::Block& getBlock() const { return block_; };

    const blockchain::Transaction& getTransaction() const { return transaction_; };

    const std::vector<blockchain::UTXO>& getUTXOlist() const { return utxolist_; };

    const MsgContent getMessageContentType() const { return content_type_; };

    int getSenderId() const { return sender_id_; };

    int getRound() const { return round_; };

    bool isEmpty() const { return empty_; };

    friend std::ostream& operator<<(std::ostream& o, const Message& m);

    friend std::string Msgtype2String(MsgType type);

    chat::Message toProtoMessage() const;

   private:
    MsgType type_;
    blockchain::Block block_;
    blockchain::Transaction transaction_;
    std::vector<blockchain::UTXO> utxolist_;
    uint64_t sender_id_;
    uint32_t round_;
    bool empty_;
    MsgContent content_type_;
};

}  // namespace blockchain
#endif
