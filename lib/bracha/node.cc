#include "node.h"

using namespace blockchain;
using namespace bracha;



bracha::Node::Node(const std::string& name, uint16_t id, uint16_t port,
  	comms::ChatServiceImpl* server):
  	//, comms::ChatClientImpl* client):
  	name_(name), 
  	id_(id), 
  	port_(port), 
  	is_faulty_(false), 
  	is_lead_(false), 
  	round_(1),
  	client_(),
  	server_(server)
{
  std::cout << "Node " << id_ << " starts!" << std::endl;
  send_this_round_ = "";
};

void bracha::Node::Run() {
  // opening a new thread for running the server
  //std::thread server_thread(&comms::ChatServiceImpl::Run, &server_, port_);
  //server_thread.join();
}

bracha::Node::~Node(){
  ready_this_round_.clear();
  echo_this_round_ .clear();
  send_this_round_ = "";  
};


void bracha::Node::RunProtocol() {

  should_broadcast_ = true;

  while (should_broadcast_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (RunTaskInLoopAndWait()) {
      //logging::cout << "Node " << id_ << " executing one round" << std::endl;
      broadcastRound(round_);
      incrementRound();
    }
  };

}



bool bracha::Node::RunTaskInLoopAndWait() {
  std::vector<Message> msg_list;
  if (server_->getMessages(msg_list)) {
    updateRecvMessages(msg_list);
    return true;
  }
  return false;
}


//iterate the msg list, add to send, ready and echo.
void bracha::Node::updateRecvMessages(const std::vector<Message>& msg_list) {

  //clear the <SEND> so that it does not send multiple ECHO to the same SEND.
  send_this_round_ = "";

  for (auto const& msg: msg_list) {
    std::string blk_hash = msg.getBlock().getHash();
    
    //update the block llist
    if (blk_list_.find(blk_hash) == blk_list_.end()) {
      blk_list_[blk_hash] = msg.getBlock();
    }
    
    switch (msg.getType()) {
      case MsgType::SEND: {
         //logging::out << id_ <<" recvs SEND frm "  << msg.getSenderId() << std::endl;
        // Recvs a <SEND, block>, broadcast echo.
    	send_this_round_ = blk_hash;
      }
      case MsgType::ECHO: {
         //logging::out << id_ <<" recvs ECHO frm "  << msg.getSenderId() << std::endl;
        if (echo_this_round_.find(blk_hash) != echo_this_round_.end()) {
          echo_this_round_[blk_hash]++;
        }
        else
          echo_this_round_[blk_hash] = 0;
      }
      case MsgType::READY: {
        //logging::out << id_ <<" recvs READY frm "  << msg.getSenderId() << std::endl;
        if (ready_this_round_.find(blk_hash) != ready_this_round_.end()) 
          ready_this_round_[blk_hash]++;
        else
          ready_this_round_[blk_hash] = 0;
      }
      default:
        continue;
    }
  }

}


void bracha::Node::broadcastRound(uint16_t round) {

  this->should_broadcast_ = true;
  bool recv_send = false;
  
  // check whether conditions on each round are satisified or not.
  if (send_this_round_ != "") {
    std::cout << id_ <<" sends ECHO in Round " << round_ << std::endl;
    broadcastMessage(blockchain::Message(MsgType::ECHO, 
    					 blk_list_[send_this_round_],
    					 this->id_, 
    					 2));
    send_this_round_= "";
  }
 
  // threshold 1
  for (const auto& [blk_hash, nb_recv] : echo_this_round_) {
    if ((nb_recv >= floor((n_ + f_ + 1) / 2)) && !ready_sent_.count(blk_hash)) {
      std::cout << id_ <<" sends 1-st READY in Round " << round_  << std::endl;
      broadcastMessage(blockchain::Message(MsgType::READY, 
                        		   blk_list_[blk_hash],
                        		   this->id_,
                        		   2));
      ready_sent_.insert(blk_hash);
    }
  }
  
  // threshold 2
  for (const auto& [blk_hash, nb_recv] : ready_this_round_) {
   
    if ( (nb_recv >= f_ + 1) && !ready_sent_.count(blk_hash)) {
      std::cout << id_ <<" sends 2-nd READY in Round" << round_ << std::endl;
      broadcastMessage(blockchain::Message(MsgType::READY, 
                        		   blk_list_[blk_hash],
                        		   this->id_,
                        		   3));
      ready_sent_.insert(blk_hash);
    }

    // delivering conditions
    if (nb_recv > 2 * bracha::f_ + 1) {
      std::cout << id_ << " delivering conditions reached in ROUND " << round_ << "!" << std::endl;
      this->should_broadcast_ = false;
      this->voted_hash_ = blk_hash;
      break;
    }

  }

}



void bracha::Node::broadcastMessage(const blockchain::Message& msg) {
  client_.Broadcast(msg);
  // logging::out << "Broadcasting message: " << msg << "\n";
}


std::ostream& operator<<(std::ostream& o, const bracha::Node& obj) {

  // Outputs the basic information of the node.
  o << "Node's ID is: " << obj.getId() << ";" 
    << "name is: "      << obj.getName() << ";" << "\n";
    
  // Outputs the nodes' accepted node.
  if (obj.isConsensus()) {
    o << "Node has accepted the node " << obj.getVotedHash() << "\n";
  }
  else {
    o << "Node has not accepted any nodes yet!" << "\n";
  }
  
  return o;  
};



bracha::LeaderNode::LeaderNode(const std::string& name, uint64_t id, uint16_t port, 
	comms::ChatServiceImpl* server) {
  name_ = name;
  id_ = id;
  port_ = port;
  is_faulty_ = false;
  is_lead_ = true; 
  round_ = 1;
  send_this_round_ = "";
  server_ = server;
  
  //Run();
};


void bracha::LeaderNode::RunProtocol() {
  should_broadcast_ = true;
  // send <SEND,block>
  propose(); 

  while (should_broadcast_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (RunTaskInLoopAndWait()) {
      broadcastRound(round_);
      incrementRound();
    }
  };
  
}

void bracha::LeaderNode::propose() {
  Block blk = Block(blk_id_++);
  blockchain::Message msg = Message(blockchain::MsgType::SEND, 
  				    blk,
  				    id_,
  				    round_);
  broadcastMessage(msg);
};



std::string bracha::ByzantineBehavior2String(bracha::ByzantineBehavior b) {
  switch (b) {
    case bracha::ByzantineBehavior::CRASH:
      return "crash";
    case bracha::ByzantineBehavior::DELAY_SEND:
      return "delay_send";
    //case bracha::ByzantineBehavior::PARTIAL_SEND:
    //  return "partial_send";
    case bracha::ByzantineBehavior::WRONG_ORDER:
      return "wrong_order";
    case bracha::ByzantineBehavior::IMPERSONATE:
      return "impersonate";
    default:
      throw std::invalid_argument("unexpected Byzantine Behavior value");
  }
};


bracha::ByzantineBehavior bracha::String2ByzantineBehavior(std::string str) {
  if (str == "crash")
      return bracha::ByzantineBehavior::CRASH;
  if (str == "delay_send")
      return bracha::ByzantineBehavior::DELAY_SEND;
    //if (str ==  "partial_send")
    //  return bracha::ByzantineBehavior::PARTIAL_SEND;
  if (str ==  "wrong_order")
      return bracha::ByzantineBehavior::WRONG_ORDER;
  if (str == "impersonate")
      return bracha::ByzantineBehavior::IMPERSONATE;
   
  throw std::invalid_argument("unexpected Byzantine Behavior value");
}




bracha::FaultNode::FaultNode(const std::string& name, 
			     uint16_t id,
			     uint16_t port,
			     bool is_lead,
			     const std::string& faulty,
			     comms::ChatServiceImpl* server) {
  name_ = name;
  id_ = id;
  port_ = port;
  is_faulty_ = true;
  faulty_ = String2ByzantineBehavior(faulty);
  is_lead_ = is_lead; 
  round_ = 1;
  send_this_round_ = "";
  server_ = server;
   
  //Run();
};


void bracha::FaultNode::RunProtocol() {


  if (ExhibitsBehavior(ByzantineBehavior::CRASH)) {
    std::cout << id_ << "crashed..." << std::endl;
    return;
  }

  should_broadcast_ = true;

  while (should_broadcast_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (RunTaskInLoopAndWait()) {
      broadcastRound(round_);
      incrementRound();
    }
  };

}



void bracha::FaultNode::broadcastRound(uint16_t round) {

  this->should_broadcast_ = true;
  bool recv_send = false;
  
  // check whether conditions on each round are satisified or not.

  if (send_this_round_ != "") {
    std::cout << id_ <<" sends ECHO in the " << round_ << " round" << std::endl;
    
    //delaying the message by delay_ ms.
    if (ExhibitsBehavior(ByzantineBehavior::DELAY_SEND)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
    }
    
    //impersonate other random node.
    uint16_t send_id = this->id_;
    if (ExhibitsBehavior(ByzantineBehavior::IMPERSONATE)) {
      send_id = (std::rand() % (n_ + 1));
    }
    
    //reverse the order of message.
    MsgType type = MsgType::ECHO;
    if (ExhibitsBehavior(ByzantineBehavior::WRONG_ORDER)) {
       type = MsgType::READY;
    }
     
    broadcastMessage(blockchain::Message(type, 
    					 blk_list_[send_this_round_],
    					 send_id,
    					 2));
    send_this_round_= "";
  }
 


  for (const auto& [blk_hash, nb_recv] : echo_this_round_) {
    // threshold 1
    if ((nb_recv >= floor((n_ + f_ + 1) / 2)) && !ready_sent_.count(blk_hash)) {
      std::cout << id_ <<" sends 1-st READY in Round " << round_ << std::endl;
      
      if (ExhibitsBehavior(ByzantineBehavior::DELAY_SEND)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
      }
      
      uint16_t send_id = this->id_;
      if (ExhibitsBehavior(ByzantineBehavior::IMPERSONATE)) {
        send_id = (std::rand() % (n_ + 1));
      }
      
      MsgType type = MsgType::READY;
      if (ExhibitsBehavior(ByzantineBehavior::WRONG_ORDER)) {
        type = MsgType::ECHO;
      }
      
    
      broadcastMessage(blockchain::Message(type,
      					   blk_list_[blk_hash],
      					   send_id,
      					   2));
                        
      ready_sent_.insert(blk_hash);
    }
  }

  for (const auto& [blk_hash, nb_recv] : ready_this_round_) {
    // threshold 2
    if ( (nb_recv >= f_ + 1) && !ready_sent_.count(blk_hash)) {
      std::cout << id_ <<" sends 2-nd READY in Round " << round_ << std::endl;
      
      if (ExhibitsBehavior(ByzantineBehavior::DELAY_SEND)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
      }
      
      uint16_t send_id = this->id_;
      if (ExhibitsBehavior(ByzantineBehavior::IMPERSONATE)) {
        send_id = (std::rand() % (n_ + 1));
      }
      
      MsgType type = MsgType::READY;
      if (ExhibitsBehavior(ByzantineBehavior::WRONG_ORDER)) {
        type = MsgType::ECHO;
      }
      
      broadcastMessage(blockchain::Message(type,
      					   blk_list_[blk_hash],
      					   send_id,
      					   3));
                        
      ready_sent_.insert(blk_hash);
    }

    // delivering conditions
    if (nb_recv > 2 * bracha::f_ + 1) {
      std::cout << id_ << " delivering conditions reached in ROUND " << round_ << "!!" << std::endl;
      this->should_broadcast_ = false;
      this->voted_hash_ = blk_hash;
      break;
    }

  }

}



