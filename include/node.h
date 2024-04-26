#ifndef COSICOIN_INCLUDE_NODE_H
#define COSICOIN_INCLUDE_NODE_H


#include <iostream>
#include <string>
#include <ostream>
#include <vector>
#include <stdbool.h>
#include <math.h>
#include <cstdint>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <map>
#include <unordered_map>
#include <set>


#include "block.h"
#include "message.h"
#include "server.h"
#include "client.h"
#include "logging.h"



namespace bracha {

const uint64_t n_ = 5;
const uint64_t f_ = 1;


class Node {
  public:
    Node() {};
    Node(const std::string& name, uint16_t id, uint16_t port, comms::ChatServiceImpl* server);
    Node(const std::string& name, uint16_t id, uint16_t port, bool is_lead);
    ~Node();

  
    /*
     * @brief runs the bracha protocol
     *
     * @param 
     * @return 
     */
    virtual void RunProtocol();
    
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
    virtual bool isLeader() const {return is_lead_; }

    /*
     * @brief check whether the node has accepted any nodes yet.
     *
     * @param 
     * @return false the node did not accept.
     * @return true the node accepted.
     */
    bool isConsensus() const {return !should_broadcast_;}


    
    std::string getName() const { return this->name_;}

    uint64_t getId() const { return this->id_;}

    // Returns the hash of the accepted block.
    std::string getVotedHash() const { return voted_hash_ ;}
  
    // Allow streaming of a node's basic information on ostreams.
    friend std::ostream& operator<<(std::ostream& o, const Node& obj);

  protected:
    // Start the server and client.
    void Run();

    // Update the storage of recv msgs.
    void updateRecvMessages(const std::vector<blockchain::Message>& msg_list);

    
    bool RunTaskInLoopAndWait();

  protected:
    // Includes two instances of the chat client and chat servers.
    comms::ChatServiceImpl* server_;
    comms::ChatClientImpl client_;

  protected:
    std::string name_;
    uint16_t id_;
    bool is_faulty_;
    uint16_t port_;
    bool is_lead_;
    
    // mapping <BLOCK_HASH, BLOCK>
    std::unordered_map<std::string,blockchain::Block> blk_list_;         
    
    // Whether the node has received a <SEND> this round.
    std::string send_this_round_ = "";    
    
    // mapping <READY_BLOCK_HASH, COUNT>                               
    std::unordered_map<std::string,uint64_t> ready_this_round_; 	  
    
    // mapping <ECHO_BLOCK_HASH, COUNT>    
    std::unordered_map<std::string,uint64_t> echo_this_round_;            
    
    // the list of the sent <READY>s.
    std::set<std::string> ready_sent_;                                   

    // the hash of the accepted block.
    std::string voted_hash_;
    bool should_broadcast_ = true;
    uint16_t round_;

    inline void incrementRound() {
      round_++;
      logging::out << "moving to round " << round_ << "\n";
    }

    
};


class LeaderNode: public Node {
  public: 
    LeaderNode(const std::string& name, uint64_t id, uint16_t port, comms::ChatServiceImpl* server);
    ~LeaderNode() {};

    bool isLeader() {return true;}

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
    uint64_t blk_id_; //TODO
};



// Represents four types of malicious behavior.

enum class ByzantineBehavior {
  NONE = 0,
  CRASH = 1 << 0,
  DELAY_SEND = 1 << 1,
  WRONG_ORDER = 1 << 2,
  IMPERSONATE = 1 << 3,
  //PARTIAL_SEND = 1 << 4,
};

// Converts string (back) to byzantine behaviour.
ByzantineBehavior String2ByzantineBehavior(std::string str);
std::string ByzantineBehavior2String(ByzantineBehavior b);

//@Override of operater &=, used for future
inline ByzantineBehavior& operator&=(ByzantineBehavior& a,
    				     ByzantineBehavior b) {
   return (ByzantineBehavior&)((int&) a &= (int)b);
}

//@Override of operater &
inline ByzantineBehavior operator&(ByzantineBehavior a,
    				   ByzantineBehavior b) {
   return (ByzantineBehavior)((int) a & (int)b);
}


/*
 * @brief compare if a malicious node includes a byzantine behaviour b.
 *
 * @param byzantine behaviour b, byzantine behaviour test
 * @return true if includes
 * @return false if not
 */       
inline bool Exhibits(ByzantineBehavior b, ByzantineBehavior test) { 
   return (b&test)!= ByzantineBehavior::NONE;
}



class FaultNode: public Node {
  public:
  
    FaultNode(const std::string& name, uint16_t id, uint16_t port, bool is_lead, const std::string& faulty, comms::ChatServiceImpl* server);
    ~FaultNode() {} ;

  public:

    // for now, we only can define one malicious behaviour in one faulty node.
    void RunProtocol();
    
    /*
     * @brief force the node to turn down after executing the current round.
     *
     * @param 
     * @return 
    */
    void Stop() {should_broadcast_ = false;}
    
  protected:  
  
    // Actions of the round.
    void broadcastRound(uint16_t round);
    
    inline bool ExhibitsBehavior(ByzantineBehavior b) const { return Exhibits(faulty_, b); }
   

  private:
    // TODO: in case we have a faulty leader.
    void propose() {};

  protected:
    // uses hot encoding,
    // which enables the combination of the (multiple) faulty behaviours.
    ByzantineBehavior faulty_;
    
    //the delaying constant(unit:millisecond)
    const int delay_ = 700;
    
};

}

#endif
