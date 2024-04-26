#ifndef COSICOIN_COMMS_CLIENT_H
#define COSICOIN_COMMS_CLIENT_H

#include <chat.grpc.pb.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <condition_variable>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "message.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using chat::Ack;
using chat::Chat;
using chat::Send;

namespace comms {

class ChatClientImpl {
   public:
    /**
     * @brief Default constructor for the ChatClientImpl class.
     *
     * Reads configured servers from servers.txt file
     */
    ChatClientImpl();

    // Destructor
    ~ChatClientImpl();

    /**
     * @brief Broadcasts a message to the network.
     *
     * Sends the specified message to all configured servers.
     *
     * @param message The message to be broadcasted.
     * @return An integer indicating the status of the broadcast.
     */
    int Broadcast(const blockchain::Message message);

   private:
    // Vector to keep server addresses
    std::vector<std::string> servers_;

    // Queues of messages that need to be send to the servers
    // One queue per server
    std::vector<std::shared_ptr<std::queue<blockchain::Message>>> message_queues_;

    // Condition variable to notify threads a new message is added to the queues
    std::condition_variable mq_cv_;

    // Mutex for the queues
    std::mutex mq_mtx_;

    // std::mutex io_mtx_;

    // Thread pool for sending threads
    std::vector<std::thread> thread_pool_;

    bool stop_threads_;
    std::mutex st_mtx_;

    // Read server addresses from text file
    int ReadServerFile_(const std::string filepath);

    // Run one thread for one server and give it it's queue
    void RunThread_(const std::string server_address, std::shared_ptr<std::queue<blockchain::Message>> mq);

    // Send message to a specific server
    int SendMessage_(const blockchain::Message message, const std::string server_address);
};

// Class to handle the GRPC client itself
class ChatClient {
   public:
    explicit ChatClient(std::shared_ptr<Channel> channel) : stub_(Chat::NewStub(channel)){};

    void Talk(const blockchain::Message &message);

   private:
    struct AsyncClientCall {
        Ack reply;
        ClientContext context;
        Status status;

        std::unique_ptr<ClientAsyncResponseReader<Ack>> response_reader;
    };

    std::unique_ptr<Chat::Stub> stub_;
    CompletionQueue cq_;
};
}  // namespace comms

#endif