#ifndef COSICOIN_INCLUDE_NODE_H
#define COSICOIN_INCLUDE_NODE_H

#include <math.h>
#include <stdbool.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "blockchain/block.h"
#include "blockchain/message.h"
#include "bracha/logging.h"
#include "comms/client.h"
#include "comms/server.h"
#include "config/settings.h"
#include "signature/hash.h"

namespace bracha {

class Node {
   public:
    Node(const std::string& name, uint16_t id, config::Settings settings, comms::ChatServiceImpl* server, std::mutex* io_mutex, bool is_lead = false) : name_(name), id_(id), is_faulty_(false), is_lead_(is_lead), grpcServer_(server), grpcClient_(settings), my_next_key_pair_(signature::Signature::getInstance()), mutex_io_(io_mutex) {
        send_this_round_ = "";
        port_ = settings.getValidatorInfo(id).port;
        cond_grpc_ = server->getConditionVariable();
        my_next_key_pair_.KeyGen();
        n_ = settings.getTotalNumberOfValidators();
        f_ = settings.getNumberOfFaultyValidators();
    }
    // Copy constructor
    Node(const Node& other) : name_(other.name_),
                              id_(other.id_),
                              grpcServer_(other.grpcServer_),
                              grpcClient_(other.grpcClient_),
                              cond_grpc_(other.cond_grpc_),
                              mutex_io_(other.mutex_io_),
                              is_faulty_(other.is_faulty_),
                              port_(other.port_),
                              is_lead_(other.is_lead_),
                              blk_list_(other.blk_list_),
                              send_this_round_(other.send_this_round_),
                              ready_this_round_(other.ready_this_round_),
                              echo_this_round_(other.echo_this_round_),
                              ready_sent_(other.ready_sent_),
                              utxolist_(other.utxolist_),
                              wallet_ids_(other.wallet_ids_),
                              voted_hash_(other.voted_hash_),
                              should_broadcast_(other.should_broadcast_),
                              my_current_private_key_(other.my_current_private_key_),
                              my_current_public_key_(other.my_current_public_key_),
                              my_next_key_pair_(other.my_next_key_pair_),
                              val_sig_keys_(other.val_sig_keys_),
                              verified_blocks_(other.verified_blocks_),
                              blocks_(other.blocks_),
                              accepted_blocks_(other.accepted_blocks_) {}

    // // Move constructor
    Node(Node&& other) noexcept : name_(std::move(other.name_)),
                                  id_(std::move(other.id_)),
                                  grpcServer_(std::move(other.grpcServer_)),
                                  grpcClient_(std::move(other.grpcClient_)),
                                  cond_grpc_(std::move(other.cond_grpc_)),
                                  mutex_io_(std::move(other.mutex_io_)),
                                  is_faulty_(std::move(other.is_faulty_)),
                                  port_(std::move(other.port_)),
                                  is_lead_(std::move(other.is_lead_)),
                                  blk_list_(std::move(other.blk_list_)),
                                  send_this_round_(std::move(other.send_this_round_)),
                                  ready_this_round_(std::move(other.ready_this_round_)),
                                  echo_this_round_(std::move(other.echo_this_round_)),
                                  ready_sent_(std::move(other.ready_sent_)),
                                  utxolist_(std::move(other.utxolist_)),
                                  wallet_ids_(std::move(other.wallet_ids_)),
                                  voted_hash_(std::move(other.voted_hash_)),
                                  should_broadcast_(std::move(other.should_broadcast_)),
                                  my_current_private_key_(std::move(other.my_current_private_key_)),
                                  my_current_public_key_(std::move(other.my_current_public_key_)),
                                  my_next_key_pair_(std::move(other.my_next_key_pair_)),
                                  val_sig_keys_(std::move(other.val_sig_keys_)),
                                  verified_blocks_(std::move(other.verified_blocks_)),
                                  blocks_(std::move(other.blocks_)),
                                  accepted_blocks_(std::move(other.accepted_blocks_)) {}

    ~Node() {
        should_broadcast_ = false;
        ready_this_round_.clear();
        echo_this_round_.clear();
        send_this_round_ = "";
    }

    /**
     * @brief runs the bracha protocol
     *
     * @param
     * @return
     */
    void RunProtocol(bool broadcast);

    /*
     * Updates the utxolists in the server
     */
    void setUTXOlists(blockchain::UTXOlist utxolist, std::vector<uint32_t> wallet_ids) {
        utxolist_ = utxolist;
        wallet_ids_ = wallet_ids;
    }

    /*
     * Clears all saved utxolists
     */
    void clearUTXOlists() { utxolist_.clear(); }

    /**
     * @brief check whether the node is a leader or not.
     *
     * @param
     * @return false the node is not leader.
     * @return true the node is leader.
     */
    inline virtual bool isLeader() const { return is_lead_; }

    /**
     * @brief check whether the node has accepted any nodes yet.
     *
     * @param
     * @return false the node did not accept.
     * @return true the node accepted.
     */
    inline bool isConsensus() const {
        std::lock_guard<std::mutex> lock(mutex_cons_);
        return !should_broadcast_;
    }

    inline std::string getName() const { return this->name_; }

    inline uint64_t getId() const { return this->id_; }

    inline std::string getVotedHash() const {
        std::lock_guard<std::mutex> lock(mutex_cons_);
        return voted_hash_;
    }

    std::vector<blockchain::Block> getConsensusBlocks(bool erase = false);

    /*
     * Returns the block for the given block hash
     * Returns empty block if hash not found
     */
    blockchain::Block getReceivedBlock(std::string block_hash);

    // Allow streaming of a node's basic information on ostreams.
    inline friend std::ostream& operator<<(std::ostream& o, const Node& obj) {
        // Outputs the basic information of the node.
        o << "Node's ID is: " << obj.getId() << ";"
          << "name is: " << obj.getName() << ";" << "\n";
        // Outputs the nodes' accepted node.
        if (obj.isConsensus()) {
            o << "Node has accepted the node " << obj.getVotedHash() << "\n";
        } else {
            o << "Node has not accepted any nodes yet!" << "\n";
        }
        return o;
    }

   protected:
    /**
     * @brief broadcast <TYPE, block> to all the existing nodes.
     *
     * @param a messgae
     * @return
     */
    virtual void broadcastMessage(blockchain::MsgType type, blockchain::Block block, uint16_t round);

    /**
     * @brief performs the protocol actions of the current round.
     *
     * @param current round
     * @return
     */
    virtual void broadcastRound();

   protected:
    // update the data structures
    void updateRecvMessages(const std::vector<blockchain::Message> msg_list);

    bool RunTaskInWait();

    /*
     * Copies next private sigkey to current private sigkey
     * Creates new next sigkey
     * Puts next public sigkey in block
     * Signs block with current sigkey
     */
    void signBlock(blockchain::Block& block);

    // bool verifyBlock(blockchain::Block& block);

    /*
     * Checks the signature of the block & stores the next sigkey
     * Returns true if signature is valid, false otherwise
     */
    bool checkBlockSig(const blockchain::Block block);

   protected:
    // Includes two instances of the chat client and chat servers.
    comms::ChatServiceImpl* grpcServer_;
    comms::ValidatorClientImpl grpcClient_;

    std::condition_variable* cond_grpc_;
    std::mutex mutex_grpc_;
    std::mutex* mutex_io_;
    mutable std::mutex mutex_cons_;

   protected:
    std::string name_;
    uint16_t id_;
    bool is_faulty_;
    uint16_t port_;

    bool is_lead_;
    uint16_t n_;
    uint16_t f_;

    std::unordered_map<std::string, blockchain::Block> blk_list_;
    std::string send_this_round_ = "";                            // list<block_hash>
    std::unordered_map<std::string, uint64_t> ready_this_round_;  // map<block_hash, list<node_id>>
    std::unordered_map<std::string, uint64_t> echo_this_round_;   // map<block_hash, list<node_id>>
    std::set<std::string> ready_sent_;                            // list<block_hash>
    blockchain::UTXOlist utxolist_;
    std::vector<uint32_t> wallet_ids_;

    std::string voted_hash_;
    bool should_broadcast_ = true;

    signature::SigKey my_current_private_key_;
    signature::SigKey my_current_public_key_;
    signature::Signature my_next_key_pair_;
    std::unordered_map<uint16_t, signature::SigKey> val_sig_keys_;  // map<node_id, val_sig_pub_key> stores the next public signing key of each validator
    std::set<std::string> verified_blocks_;                         // stores the hash of each verified block
    std::unordered_map<std::string, blockchain::Block> blocks_;     // map<block_id, block> stores all received blocks with all their signatures
    std::vector<std::string> accepted_blocks_;                      // stores the hash of each accepted block
};

class LeaderNode : public Node {
   public:
    LeaderNode(const std::string& name, uint64_t id, config::Settings settings, comms::ChatServiceImpl* server, std::mutex* io_mutex) : Node(name, id, settings, server, io_mutex, true){};
    // Copy constructor
    LeaderNode(const LeaderNode& other) : Node(other) {}
    // Move constructor
    LeaderNode(LeaderNode&& other) noexcept : Node(std::move(other)) {}

    ~LeaderNode(){};

    bool isLeader() { return true; }

   public:
    // @ override
    void RunProtocol(bool broadcast);

    /**
     * @brief propose a new block to all the existing nodes.
     *
     * @param NONE
     * @return
     */
    int Propose(blockchain::Block& bk);
};

// Represents different types of malicious behavior.
enum class ByzantineBehavior {
    NONE,
    CRASH,
    DELAY_SEND,
    PARTIAL_SEND,
    WRONG_ORDER,
    IMPERSONATE,
};

class FaultNode : public Node {
   public:
    FaultNode(const std::string& name, uint64_t id, config::Settings settings, comms::ChatServiceImpl* server, std::mutex* io_mutex) : Node(name, id, settings, server, io_mutex, false) {
        is_faulty_ = true;
    };

    ~FaultNode(){};

   public:
    std::string byzantineBehavior2String(ByzantineBehavior b) {
        switch (b) {
            case bracha::ByzantineBehavior::CRASH: {
                return "crash";
                break;
            }
            case bracha::ByzantineBehavior::DELAY_SEND: {
                return "delay_send";
                break;
            }
            case bracha::ByzantineBehavior::PARTIAL_SEND: {
                return "partial_send";
                break;
            }
            case bracha::ByzantineBehavior::WRONG_ORDER: {
                return "wrong_order";
                break;
            }
            case bracha::ByzantineBehavior::IMPERSONATE: {
                return "impersonate";
                break;
            }
            default:
                throw std::invalid_argument("unexpected Byzantine Behavior value");
        }
    }

    /*
     * Run faulty node with defined behaviour
     * Already implemented:
     * - CRASH
     */
    void RunProtocol(bool broadcast, bracha::ByzantineBehavior behaviour);

   protected:
    /**
     * @brief broadcast <TYPE, block> to all the existing nodes.
     *
     * @param a messgae
     * @return
     */
    void broadcastMessage(const blockchain::Message& msg) const {};

    // Actions of the round.
    void broadcastRound() {};

   private:
    // TODO: in case we have a faulty leader.
    int Propose(const blockchain::Block& bk) { return 0; }

   protected:
    // the malicious behaviours using hot encoding, which enables the combination of the defined malicious behaviours.
    unsigned int faulty_;
};

}  // namespace bracha

#endif
