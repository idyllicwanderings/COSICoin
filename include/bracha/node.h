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

namespace bracha {

const uint64_t n_ = 3;
const uint64_t f_ = 0;

class Node {
   public:
    Node(){};
    Node(const std::string& name, uint16_t id, uint16_t port, comms::ChatServiceImpl* server);
    Node(const std::string& name, uint16_t id, uint16_t port, bool is_lead);
    ~Node();

    /*
     * @brief runs the bracha protocol
     *
     * @param
     * @return
     */
    void RunProtocol();

   protected:
    /*
     * @brief broadcast <TYPE, block> to all the existing nodes.
     *
     * @param a messgae
     * @return
     */
    virtual void broadcastMessage(const blockchain::Message& msg);

    /*
     * @brief performs the protocol actions of the current round.
     *
     * @param current round
     * @return
     */
    virtual void broadcastRound(uint16_t round);

   public:
    /*
     * @brief check whether the node is a leader or not.
     *
     * @param
     * @return false the node is not leader.
     * @return true the node is leader.
     */
    virtual bool isLeader() const { return is_lead_; }

    /*
     * @brief check whether the node has accepted any nodes yet.
     *
     * @param
     * @return false the node did not accept.
     * @return true the node accepted.
     */
    bool isConsensus() const { return !should_broadcast_; }

    std::string getName() const { return this->name_; }

    uint64_t getId() const { return this->id_; }

    std::string getVotedHash() const { return voted_hash_; }

    // Allow streaming of a node's basic information on ostreams.
    friend std::ostream& operator<<(std::ostream& o, const Node& obj);

   protected:
    // Start the server and client.
    void Run();

    // update the data structures
    void updateRecvMessages(const std::vector<blockchain::Message>& msg_list);

    bool RunTaskInLoopAndWait();

   protected:
    // Includes two instances of the chat client and chat servers.
    // UNCOMMENT
    comms::ChatServiceImpl* server_;
    comms::ChatClientImpl client_;

   protected:
    std::string name_;
    uint16_t id_;
    bool is_faulty_;
    uint16_t port_;

    bool is_lead_;

    std::unordered_map<std::string, blockchain::Block> blk_list_;
    std::string send_this_round_ = "";                            // list<block_hash>
    std::unordered_map<std::string, uint64_t> ready_this_round_;  // map<block_hash, list<node_id>>
    std::unordered_map<std::string, uint64_t> echo_this_round_;   // map<block_hash, list<node_id>>
    std::set<std::string> ready_sent_;                            // list<block_hash>

    std::string voted_hash_;
    bool should_broadcast_ = true;
    uint16_t round_;

    inline void incrementRound() {
        round_++;
        logging::out << "moving to round " << round_ << "\n";
    }
};

class LeaderNode : public Node {
   public:
    LeaderNode(const std::string& name, uint64_t id, uint16_t port, comms::ChatServiceImpl* server);
    //, comms::ChatServiceImpl* server);
    ~LeaderNode(){};

    bool isLeader() { return true; }

   public:
    // @ override
    void RunProtocol();

   private:
    /*
     * @brief propose a new block to all the existing nodes.
     *
     * @param NONE
     * @return
     */
    void propose();

   private:
    uint64_t blk_id_;  // TODO
};

// Represents different types of malicious behavior.
enum class ByzantineBehavior {
    NONE = 0,
    CRASH = 1 << 0,
    DELAY_SEND = 1 << 1,
    PARTIAL_SEND = 1 << 2,
    WRONG_ORDER = 1 << 3,
    IMPERSONATE = 1 << 4,
};

class FaultNode : public Node {
   public:
    FaultNode(const std::string& name, uint16_t id, uint16_t port, bool is_lead);
    ~FaultNode(){};

   public:
    friend ByzantineBehavior string2ByzantineBehavior(std::string str);
    friend std::string byzantineBehavior2String(ByzantineBehavior b);

    // TODO: three malicious functions need to be implemented.
    void RunProtocol() {};

   protected:
    /*
     * @brief broadcast <TYPE, block> to all the existing nodes.
     *
     * @param a messgae
     * @return
     */
    void broadcastMessage(const blockchain::Message& msg) const {};

    // Actions of the round.
    void broadcastRound(uint16_t round) {};

   private:
    // TODO: in case we have a faulty leader.
    void propose() {};

   protected:
    // the malicious behaviours using hot encoding, which enables the combination of the defined malicious behaviours.
    unsigned int faulty_;
};

}  // namespace bracha

#endif
