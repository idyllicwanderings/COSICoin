#include <chrono>
#include <thread>

#include "server.h"

void print_message(blockchain::Message msg) {
    std::cout << "Got message, transaction: " << msg.getBlock().getTransaction() << std::endl;
}

void print_messages(const std::vector<blockchain::Message>& messages) {
    for (const auto& msg : messages) {
        std::cout << "Got messages: " << msg.getBlock().getTransaction() << ", ";
    }
    std::cout << std::endl;
}

int main() {
    comms::ChatServiceImpl server1;
    std::thread server1_thread(&comms::ChatServiceImpl::Run, &server1, 50001);
    comms::ChatServiceImpl server2;
    std::thread server2_thread(&comms::ChatServiceImpl::Run, &server2, 50002);
    comms::ChatServiceImpl server3;
    std::thread server3_thread(&comms::ChatServiceImpl::Run, &server3, 50003);

    std::vector<blockchain::Message> msg_list;
    bool new_msg;
    while (true) {
        new_msg = server1.getMessages(msg_list);
        if (new_msg) {
            std::cout << "Server 1: ";
            print_messages(msg_list);
            msg_list.clear();
        };
        new_msg = server2.getMessages(msg_list);
        if (new_msg) {
            std::cout << "Server 2: ";
            print_messages(msg_list);
            msg_list.clear();
        };
        new_msg = server3.getMessages(msg_list);
        if (new_msg) {
            std::cout << "Server 3: ";
            print_messages(msg_list);
            msg_list.clear();
        };
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
