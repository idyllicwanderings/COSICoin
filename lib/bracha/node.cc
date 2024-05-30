#include "bracha/node.h"

// #include <chrono>

using namespace blockchain;
using namespace bracha;

void bracha::Node::RunProtocol(bool broadcast) {

    std::cout << "Node " << id_ << ": Run protocol started" << std::endl;
    mutex_cons_.lock();
    should_broadcast_ = broadcast;

    while (should_broadcast_) {
        mutex_cons_.unlock();
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (RunTaskInWait()) {
            broadcastRound();
        }
        mutex_cons_.lock();
    };
    mutex_cons_.unlock();
}

bool bracha::Node::RunTaskInWait() {
    std::vector<Message> msg_list;
    std::unique_lock<std::mutex> lck(mutex_grpc_);
    cond_grpc_->wait(lck, [this]() { return grpcServer_->newConsensusMessages() || !should_broadcast_; });
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Wait for one second to make sure all messages are in correct group received (easier to debug)
    grpcServer_->getMessages(msg_list);
    mutex_grpc_.unlock();
    updateRecvMessages(msg_list);
    return true;
}

// iterate the msg list, add to send, ready and echo.
void bracha::Node::updateRecvMessages(const std::vector<Message> msg_list) {
    // clear the <SEND> so that it does not send multiple ECHO to the same SEND.
    send_this_round_ = "";

    std::cout << "Node " << id_ << ": renewing msg list #msgs: " << msg_list.size() << std::endl;
    for (blockchain::Message msg : msg_list) {
        blockchain::Block block(msg.getBlock());
        uint64_t blk_id = block.getID();
        std::string blk_hash = block.getHeader().getID();
        //mutex_io_->lock();
        std::cout << "Node " << id_ << ": received block: " << blk_id << ", " << blk_hash.substr(0, 10) << ", message type: " << msg.getType() << ", from node " << msg.getSenderId() << std::endl;
        //mutex_io_->unlock();

        // Verify the block signature
        if (!checkBlockSig(block)) {
            //mutex_io_->lock();
            std::cout << "Node " << id_ << ": block signature invalid, skipping block" << std::endl;
            //mutex_io_->unlock();
            continue;
        }

        // Verify the block
        if (!verified_blocks_.count(blk_hash)) {
            // Block not yet verified
            if (!block.verify(utxolist_, wallet_ids_)) {
                //mutex_io_->lock();
                std::cout << "Node " << id_ << ": block verification failed, skipping block" << std::endl;
                //mutex_io_->unlock();
                continue;
            } else {
                verified_blocks_.insert(blk_hash);
            }
        }

        // Save the block's signature
        uint16_t node_id = block.getValidatorID();
        blockchain::BlockSignature sig;
        sig.signature = block.getValidatorSignature();
        sig.public_key = val_sig_keys_[node_id];
        sig.validator_id = node_id;
        sig.round = msg.getRound();
        mutex_cons_.lock();
        if (blocks_.find(blk_hash) == blocks_.end()) {
            blocks_[blk_hash] = block;
        }
        blocks_[blk_hash].addValidatorSignature(sig);
        mutex_cons_.unlock();

        // update the block list
        if (blk_list_.find(blk_hash) == blk_list_.end()) {
            blk_list_[blk_hash] = block;
        }

        switch (msg.getType()) {
            case MsgType::SEND: {
                // std::cout << "Node " << id_ << ": counting SEND from " << msg.getSenderId() << std::endl;
                send_this_round_ = blk_hash;
                break;
            }
            case MsgType::ECHO: {
                // std::cout << "Node " << id_ << ": counting ECHO from " << msg.getSenderId() << std::endl;
                if (echo_this_round_.find(blk_hash) != echo_this_round_.end())
                    echo_this_round_[blk_hash]++;
                else
                    echo_this_round_[blk_hash] = 1;
                break;
            }
            case MsgType::READY: {
                // std::cout << "Node " << id_ << ": counting READY from " << msg.getSenderId() << std::endl;
                if (ready_this_round_.find(blk_hash) != ready_this_round_.end())
                    ready_this_round_[blk_hash]++;
                else
                    ready_this_round_[blk_hash] = 1;
                break;
            }
            default:
                continue;
        }
    }
}

void print_dict(std::string name, std::unordered_map<std::string, uint64_t> dict) {
    std::cout << name << ": ";
    for (const auto& [key, value] : dict) {
        std::cout << key.substr(0, 10) << ": " << value << ", ";
    }
    std::cout << std::endl;
}

void bracha::Node::broadcastRound() {
    this->should_broadcast_ = true;
    bool recv_send = false;
    // check whether conditions on each round are satisified or not.

    //mutex_io_->lock();
    print_dict("Node " + std::to_string(id_) + ": echo", echo_this_round_);
    print_dict("Node " + std::to_string(id_) + ": ready", ready_this_round_);
    //mutex_io_->unlock();

    if (send_this_round_ != "") {
        broadcastMessage(MsgType::ECHO, blk_list_[send_this_round_], 1);
        send_this_round_ = "";
    }

    for (const auto& [blk_hash, nb_recv] : echo_this_round_) {
        if ((nb_recv >= floor((n_ + f_ + 1) / 2)) && !ready_sent_.count(blk_hash)) {
            broadcastMessage(MsgType::READY, blk_list_[blk_hash], 2);
            ready_sent_.insert(blk_hash);
        }
    }

    for (const auto& [blk_hash, nb_recv] : ready_this_round_) {
        if ((nb_recv >= f_ + 1) && !ready_sent_.count(blk_hash)) {
            broadcastMessage(MsgType::READY, blk_list_[blk_hash], 3);
            ready_sent_.insert(blk_hash);
        }

        // delivering conditions
        if (nb_recv > 2 * f_ + 1) {
            std::cout << "Node " << id_ << ": Delivering conditions reached for block: " << blk_hash.substr(0, 10) << std::endl;
            mutex_cons_.lock();
            this->should_broadcast_ = false;
            this->voted_hash_ = blk_hash;
            mutex_cons_.unlock();
            // remove block from counters
            echo_this_round_.erase(blk_hash);
            ready_this_round_.erase(blk_hash);
            mutex_cons_.lock();
            accepted_blocks_.push_back(blk_hash);
            mutex_cons_.unlock();
            break;
        }
    }
}

void bracha::Node::broadcastMessage(blockchain::MsgType type, blockchain::Block block, uint16_t round) {
    signBlock(block);
    //mutex_io_->lock();
    std::cout << "Node " << id_ << ": Broadcasting block " << block.getHeader().getID().substr(0, 10) << " in message type " << type << std::endl;
    //mutex_io_->unlock();
    blockchain::Message msg = Message(type, block, id_, round);
    mutex_grpc_.lock();
    grpcClient_.Broadcast(msg);
    mutex_grpc_.unlock();
}

void bracha::LeaderNode::RunProtocol(bool broadcast) {
    mutex_cons_.lock();
    should_broadcast_ = broadcast;

    while (should_broadcast_) {
        mutex_cons_.unlock();
        if (RunTaskInWait()) {
            broadcastRound();
        }
        mutex_cons_.lock();
    };
    mutex_cons_.unlock();
}

int bracha::LeaderNode::Propose(blockchain::Block& bx) {
    std::cout << "Node.cc:  " << "propose started " << std::endl;
    broadcastMessage(blockchain::MsgType::SEND, bx, 0);
    std::cout << "Node.cc:  " << "propose ended " << std::endl;
    return 1;
}

void bracha::Node::signBlock(blockchain::Block& block) {
    my_current_private_key_ = my_next_key_pair_.getPrivateKey();
    my_current_public_key_ = my_next_key_pair_.getPublicKey();
    my_next_key_pair_.KeyGen();
    signature::SigKey my_next_pub_key = my_next_key_pair_.getPublicKey();
    //mutex_io_->lock();
    std::cout << "Node " << id_ << ": signing block " << block.getHeader().getID().substr(0, 10) << " for pub key: " << signature::print_SigKey(my_current_public_key_) << " with next key: " << signature::print_SigKey(my_next_pub_key) << std::endl;
    //mutex_io_->unlock();
    block.sign(my_current_private_key_, id_, my_next_pub_key);
    assert(signature::Verify(block.getDigest(), block.getValidatorSignature(), my_current_public_key_));
}

bool bracha::Node::checkBlockSig(const blockchain::Block blk) {
    blockchain::Block block(blk);
    int valid = 1;  // default: accept signature if no public key known
    std::vector<std::string> signature = block.getValidatorSignature();
    uint16_t node_id = block.getValidatorID();
    std::string block_digest = block.getDigest();
    // Check if we have the pub key for this node
    if (val_sig_keys_.find(node_id) != val_sig_keys_.end()) {
        // Node ID found in the map, we can check the signature
        valid = signature::Verify(block_digest, signature, val_sig_keys_[node_id]);
        //mutex_io_->lock();
        std::cout << "Node " << id_ << ": signature from node " << node_id << " for block " << block_digest.substr(0, 10) << " with key " << signature::print_SigKey(val_sig_keys_[node_id]) << " -> valid: " << valid << std::endl;
        //mutex_io_->unlock();
    }
    // Save the next pub key
    val_sig_keys_[node_id] = block.getValidatorNextPublicKey();
    //mutex_io_->lock();
    std::cout << "Node " << id_ << ": added next pub key for node " << node_id << ": " << signature::print_SigKey(block.getValidatorNextPublicKey()) << std::endl;
    //mutex_io_->unlock();
    return valid == 1;
}

blockchain::Block Node::getReceivedBlock(std::string block_hash) {
    std::lock_guard<std::mutex> lock(mutex_cons_);
    // Check if the block is in the list of received blocks
    if (blocks_.find(block_hash) != blocks_.end()) {
        return blockchain::Block(blocks_[block_hash]);
    }
    return blockchain::Block();
}

std::vector<blockchain::Block> Node::getConsensusBlocks(bool erase) {
    std::lock_guard<std::mutex> lock(mutex_cons_);
    std::vector<blockchain::Block> blocks;
    for (std::string hash : accepted_blocks_) {
        blocks.push_back(blocks_[hash]);
    }
    if (erase) {
        accepted_blocks_.clear();  // TODO
        should_broadcast_ = false;
    }
    return blocks;
}

void bracha::FaultNode::RunProtocol(bool broadcast, bracha::ByzantineBehavior behaviour) {
    std::cout << "Faulty node " << id_ << ": Run protocol started in mode: " << byzantineBehavior2String(behaviour) << std::endl;
    mutex_cons_.lock();
    should_broadcast_ = behaviour != ByzantineBehavior::CRASH;

    while (should_broadcast_) {
        mutex_cons_.unlock();
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (RunTaskInWait()) {
            broadcastRound();
        }
        mutex_cons_.lock();
    };
    mutex_cons_.unlock();
}
