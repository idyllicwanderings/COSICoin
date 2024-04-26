#include "blockchain/message.h"

using namespace blockchain;

int Message::setType(const MsgType type_message) {
    type_ = type_message;
    return 1;
}

chat::Message Message::toProtoMessage() const {
    chat::Message proto_message;

    // Convert type
    // ECHO = 0;
    // SEND = 1;
    // READY = 2;
    if (type_ == ECHO) {
        proto_message.set_type(0);
    } else if (type_ == SEND) {
        proto_message.set_type(1);
    } else if (type_ == READY) {
        proto_message.set_type(2);
    }

    if (content_type_ == BLOCK) {
        // Convert block_
        chat::Block* proto_block = proto_message.mutable_block();
        block_.toProtoBlock(proto_block);
    } else if (content_type_ == TRANSACTION) {
        // Convert transaction_
        chat::Transaction* proto_tx = proto_message.mutable_tx();
        transaction_.toProtoTransaction(proto_tx);
    } else if (content_type_ == UTXOLIST) {
        // Convert utxolist
        chat::UTXOlist* proto_utxolist = proto_message.mutable_utxolist();
        for (int i = 0; i < utxolist_.size(); i++) {
            utxolist_[i].toProtoUTXO(proto_utxolist->add_utxo());
        }
    }

    proto_message.set_sender_id(sender_id_);
    proto_message.set_round(round_);

    return proto_message;
}

Message::Message(const chat::Message& proto_message) {
    uint32_t proto_type = proto_message.type();
    if (proto_type == 0) {
        type_ = ECHO;
    } else if (proto_type == 1) {
        type_ = SEND;
    } else if (proto_type == 2) {
        type_ = READY;
    }
    sender_id_ = proto_message.sender_id();
    round_ = proto_message.round();
    empty_ = false;
    if (proto_message.has_block()) {
        content_type_ = BLOCK;
        block_.fromProtoBlock(proto_message.block());
    }
    if (proto_message.has_tx()) {
        content_type_ = TRANSACTION;
        transaction_.fromProtoTransaction(proto_message.tx());
    }
    if (proto_message.has_utxolist()) {
        content_type_ = UTXOLIST;
        chat::UTXOlist utxolist = proto_message.utxolist();
        for (int i = 0; i < utxolist.utxo_size(); i++) {
            blockchain::UTXO utxo(utxolist.utxo(i));
            utxolist_.push_back(utxo);
        }
    }
}

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
