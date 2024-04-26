#include "block.h"

using namespace blockchain;

std::string Block::calculateHash() const {
    std::stringstream ss;
    ss << id_ << time_ << nonce_ << prev_hash_;
    //   for (const auto &transaction : transactions_) {
    //     ss << transaction.GetSender() << transaction.GetReceiver()
    //        << transaction.GetAmount();
    //   }
    // TODO: implement hash or call hash
    return ss.str();
}

chat::Block Block::toProtoBlock() const {
    chat::Block proto_block;

    proto_block.set_time(static_cast<int64_t>(time_));
    proto_block.set_transactions(transactions_);
    proto_block.set_id(id_);
    proto_block.set_prev_hash(prev_hash_);
    proto_block.set_hash(hash_);
    proto_block.set_nonce(nonce_);

    return proto_block;
}

Block::Block(const chat::Block& proto_block) {
    id_ = proto_block.id();
    time_ = static_cast<time_t>(proto_block.time());
    nonce_ = proto_block.nonce();
    prev_hash_ = proto_block.prev_hash();
    hash_ = proto_block.hash();
    transactions_ = proto_block.transactions();
    empty_ = false;
}


std::ostream& operator<<(std::ostream& o, const Block& b){
    o << "block_id: " << b.getId() << std::endl;
}

