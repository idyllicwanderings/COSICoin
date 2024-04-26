#include "node.h"
#include <thread>
#include <chrono>

using namespace bracha;

int main(){
    comms::ChatServiceImpl server1;
    //comms::ChatClientImpl client1;
    
    
    std::thread server1_thread(&comms::ChatServiceImpl::Run, &server1, 50001);
    
    LeaderNode n1 = LeaderNode("leader", 1, 50001, &server1);
    
    
    
    comms::ChatServiceImpl server2;
    std::thread server2_thread(&comms::ChatServiceImpl::Run, &server2, 50002);
    Node n2 = Node("n2", 2, 50002, &server2);
    
    
    comms::ChatServiceImpl server3;
    std::thread server3_thread(&comms::ChatServiceImpl::Run, &server3, 50003);
    Node n3 = Node("n3", 3, 50003, &server3);
    

    std::thread n1_thread(&LeaderNode::RunProtocol, &n1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::thread n2_thread(&Node::RunProtocol, &n2);
    std::thread n3_thread(&Node::RunProtocol, &n3);
   
    
    n1_thread.join();
    n2_thread.join();
    n3_thread.join();
    
    server1_thread.join();
    server2_thread.join();
    server3_thread.join();

    

    
    return 1;
}
