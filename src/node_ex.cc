#include "node.h"
#include <thread>
#include <chrono>

using namespace bracha;

void NoFaultTEST() {

    comms::ChatServiceImpl server1;    
    std::thread server1_thread(&comms::ChatServiceImpl::Run, &server1, 50001);
    LeaderNode n1 = LeaderNode("leader", 1, 50001, &server1);
    
    
    
    comms::ChatServiceImpl server2;
    std::thread server2_thread(&comms::ChatServiceImpl::Run, &server2, 50002);
    Node n2 = Node("n2", 2, 50002, &server2);
    
    
    comms::ChatServiceImpl server3;
    std::thread server3_thread(&comms::ChatServiceImpl::Run, &server3, 50003);
    Node n3 = Node("n3", 3, 50003, &server3);
    
    
    comms::ChatServiceImpl server4;
    std::thread server4_thread(&comms::ChatServiceImpl::Run, &server4, 50004);
    Node n4 = Node("n4", 4, 50004, &server4);
    
    
    comms::ChatServiceImpl server5;
    std::thread server5_thread(&comms::ChatServiceImpl::Run, &server5, 50005);
    Node n5 = Node("n5", 5, 50005, &server5);

    std::thread n1_thread(&LeaderNode::RunProtocol, &n1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::thread n2_thread(&Node::RunProtocol, &n2);
    std::thread n3_thread(&Node::RunProtocol, &n3);
    std::thread n4_thread(&Node::RunProtocol, &n4);
    std::thread n5_thread(&Node::RunProtocol, &n5);
   
    
    n1_thread.join();
    n2_thread.join();
    n3_thread.join();
    n4_thread.join();
    n5_thread.join();
    
    server1_thread.join();
    server2_thread.join();
    server3_thread.join();
    server4_thread.join();
    server5_thread.join();
}


void FaultTEST(const std::string& b) {

    comms::ChatServiceImpl server1;    
    std::thread server1_thread(&comms::ChatServiceImpl::Run, &server1, 50001);
    LeaderNode n1 = LeaderNode("leader", 1, 50001, &server1);
    
    
    
    comms::ChatServiceImpl server2;
    std::thread server2_thread(&comms::ChatServiceImpl::Run, &server2, 50002);
    Node n2 = Node("n2", 2, 50002, &server2);
    
    
    comms::ChatServiceImpl server3;
    std::thread server3_thread(&comms::ChatServiceImpl::Run, &server3, 50003);
    Node n3 = Node("n3", 3, 50003, &server3);
    
    
    comms::ChatServiceImpl server4;
    std::thread server4_thread(&comms::ChatServiceImpl::Run, &server4, 50004);
    Node n4 = Node("n4", 4, 50004, &server4);
    
    
    comms::ChatServiceImpl server5;
    std::thread server5_thread(&comms::ChatServiceImpl::Run, &server5, 50005);
    
    FaultNode n5 = FaultNode("n5", 5, 50005, false, b, &server5);

    std::thread n1_thread(&LeaderNode::RunProtocol, &n1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::thread n2_thread(&Node::RunProtocol, &n2);
    std::thread n3_thread(&Node::RunProtocol, &n3);
    std::thread n4_thread(&Node::RunProtocol, &n4);
    std::thread n5_thread(&Node::RunProtocol, &n5);
   
    
    n1_thread.join();
    n2_thread.join();
    n3_thread.join();
    n4_thread.join();
    n5_thread.join();
    
    server1_thread.join();
    server2_thread.join();
    server3_thread.join();
    server4_thread.join();
    server5_thread.join();
}

int main(){
    
    
    // the test for bracha asynchrouns protocol with no fault tolerance
    // n = 5, f = 0
    //NoFaultTEST();
    
    // the test for bracha asynchrouns protocol with up to 1/3 fault tolerance
    // n = 5, f = 1, with different scenarios
    std::string b1 = "delay_send";
    std::string b2 = "crash";
    std::string b3 = "wrong_order";
    std::string b4 = "impersonate";
    
    //FaultTEST(b1);
    //FaultTEST(b2);
    //FaultTEST(b3);
    //FaultTEST(b4);
    
    return 1;
}
