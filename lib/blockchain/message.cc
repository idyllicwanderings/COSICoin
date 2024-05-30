#include "blockchain/message.h"

using namespace blockchain;

int Message::setType(const MsgType type_message) {
    type_ = type_message;
    return 1;
}

// chat::Message Message::toProtoMessage() const {
//     chat::Message proto_message;

//     // Convert type
//     // ECHO = 0;
//     // SEND = 1;
//     // READY = 2;
//     if (type_ == MsgType::ECHO) {
//         proto_message.set_type(0);
//     } else if (type_ == MsgType::SEND) {
//         proto_message.set_type(1);
//     } else if (type_ == MsgType::READY) {
//         proto_message.set_type(2);
//     }

//     // Convert block_
//     chat::Block* proto_block = proto_message.mutable_block();
//     block_.toProtoBlock(proto_block);

//     proto_message.set_sender_id(sender_id_);
//     proto_message.set_round(round_);

//     return proto_message;
// }

// Message::Message(const chat::Message& proto_message) {
//     uint32_t proto_type = proto_message.type();
//     if (proto_type == 0) {
//         type_ = MsgType::ECHO;
//     } else if (proto_type == 1) {
//         type_ = MsgType::SEND;
//     } else if (proto_type == 2) {
//         type_ = MsgType::READY;
//     }
//     sender_id_ = proto_message.sender_id();
//     round_ = proto_message.round();
//     empty_ = false;
//     block_.fromProtoBlock(proto_message.block());
// }

std::string Msgtype2String(MsgType type) {
    switch (type) {
        case (MsgType::SEND):
            return "SEND";
        case (MsgType::ECHO):
            return "ECHO";
        case (MsgType::READY):
            return "READY";
        default:
            return "ERROR TYPE!";
    }
}

std::ostream& operator<<(std::ostream& o, const Message& m) {
    o << "sender_id: " << m.getSenderId()
      << "round: " << m.getRound()
      << "block_id: " << m.getBlock().getID()
      << "type: " << Msgtype2String(m.getType()) << std::endl;
    return o;
}
