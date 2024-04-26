#include "client.h"

int main() {
    comms::ChatClientImpl client;

    std::string input;

    while (input != "q") {
        std::cout << "Enter transaction (q to quit): ";
        std::cin >> input;

        if (input == "q") {
            break;
        }
        blockchain::Block block1(1, input + "-1");
        block1.finalize();
        blockchain::Message message1(blockchain::SEND, block1, 1, 1);
        blockchain::Block block2(1, input + "-2");
        block2.finalize();
        blockchain::Message message2(blockchain::SEND, block2, 1, 1);
        blockchain::Block block3(1, input + "-3");
        block3.finalize();
        blockchain::Message message3(blockchain::SEND, block3, 1, 1);
        blockchain::Block block4(1, input + "-4");
        block4.finalize();
        blockchain::Message message4(blockchain::SEND, block4, 1, 1);

        client.Broadcast(message1);
        client.Broadcast(message2);
        client.Broadcast(message3);
        client.Broadcast(message4);
    }
}