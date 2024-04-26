#ifndef COSICOIN_COMMS_SERVER_H
#define COSICOIN_COMMS_SERVER_H

#include <chat.grpc.pb.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "message.h"

using chat::Ack;
using chat::Chat;
using chat::Send;
using grpc::CompletionQueue;
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

namespace comms {

class ChatServiceImpl {
   public:
    ~ChatServiceImpl();

    /**
     * @brief Runs the server on the specified port.
     *
     * This function starts the server and listens for incoming connections on the specified port.
     * This function needs to be run in a separate thread, it does not return
     *
     * @param port The port number on which the server should listen.
     */
    void Run(uint16_t port);

    /**
     * @brief Appends received messages to `msg_list`
     *
     * Appends all received messages (since last call) in `msg_list`
     * Deletes these messages from internal FIFO queue
     *
     * @param msg_list The vector to store the retrieved messages.
     * @return True if there are new messages in `msg_list`, false if there are no new messages
     */
    bool getMessages(std::vector<blockchain::Message>& msg_list);

   public:
    class CallData {
       public:
        // One Calldata object handling each request thread
        CallData(Chat::AsyncService* service, ServerCompletionQueue* cq, std::queue<blockchain::Message>* mq,
                 std::mutex* io_mutex_, std::mutex* mq_mutex_);

        void Proceed();

       private:
        Chat::AsyncService* cd_service_;
        ServerCompletionQueue* cd_cq_;
        std::queue<blockchain::Message>* cd_mq_;
        std::mutex* cd_mq_mutex_;
        std::mutex* cd_io_mutex_;
        ServerContext ctx_;

        Send request_;
        Ack reply_;
        ServerAsyncResponseWriter<Ack> responder_;

        enum CallStatus { CREATE,
                          START_PROCESS,
                          FINISH };
        CallStatus status_;  // The current serving state.
    };

   private:
    void HandleRpcs();
    Chat::AsyncService service_;
    std::unique_ptr<Server> server_;
    std::unique_ptr<ServerCompletionQueue> cq_;

    std::queue<blockchain::Message> mq_;
    std::mutex mq_mutex_;
    std::mutex io_mutex_;

    std::unordered_map<uint32_t, std::vector<blockchain::Message>> messages_per_round_;
};
}  // namespace comms

#endif