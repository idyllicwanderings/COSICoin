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
#include <thread>
#include <unordered_map>
#include <vector>

#include "blockchain/message.h"
#include "blockchain/transaction.h"
#include "blockchain/utxo.h"

using chat::Chat;
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
    // ChatServiceImpl(){};
    // ChatServiceImpl(std::condition_variable* cv) : cv_(cv){};
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
     * Looks only at consensus messages
     *
     * @param msg_list The vector to store the retrieved messages.
     * @return True if there are new messages in `msg_list`, false if there are no new messages
     */
    bool getMessages(std::vector<blockchain::Message>& msg_list);

    /*
     * Checks if there are new messages for the consensus protocol
     */
    bool newConsensusMessages();

    /*
     * Checks if there are new requests from wallets
     */
    bool newRequests();

    /*
     * Appends all new transactions from wallets in `tx_list`
     * Deletes these transactions from internal FIFO queue
     * Returns True if there are new transactions in `tx_list`
     */
    bool getTransactions(std::vector<blockchain::Transaction>& tx_list);

    /*
     * Updates the utxolists in the server
     */
    void setUTXOlists(std::unordered_map<uint32_t, blockchain::UTXOlist> utxolists);

    /*
     * Clears all saved utxolists
     */
    void clearUTXOlists();

    /*
     * Returns a pointer to a condition variable that gets notified when a new message or transaction is received
     */
    std::condition_variable* getConditionVariable() { return &cv_; };

   public:
    class CallData {
       public:
        CallData(Chat::AsyncService* service, ServerCompletionQueue* cq)
            : cd_service_(service), cd_cq_(cq), status_(CREATE){};

        virtual void Proceed() = 0;

       protected:
        Chat::AsyncService* cd_service_;
        ServerCompletionQueue* cd_cq_;
        ServerContext ctx_;
        enum CallStatus { CREATE,
                          START_PROCESS,
                          FINISH };
        CallStatus status_;  // The current serving state.
    };

    class CallDataTalk : public CallData {
       public:
        // One Calldata object handling each request thread
        CallDataTalk(Chat::AsyncService* service, ServerCompletionQueue* cq, std::queue<blockchain::Message>* mq,
                     std::mutex* io_mutex, std::mutex* mq_mutex, std::condition_variable* cv);

        void Proceed() override;

       private:
        std::queue<blockchain::Message>* cd_mq_;
        std::mutex* cd_mq_mutex_;
        std::mutex* cd_io_mutex_;

        chat::Send request_;
        chat::Ack reply_;
        ServerAsyncResponseWriter<chat::Ack> responder_;

        std::condition_variable* cd_cv_ = nullptr;
    };

    class CallDataNewTx : public CallData {
       public:
        // One Calldata object handling each request thread
        CallDataNewTx(Chat::AsyncService* service, ServerCompletionQueue* cq, std::queue<blockchain::Transaction>* txq,
                      std::mutex* io_mutex, std::mutex* txq_mutex, std::condition_variable* cv);

        void Proceed() override;

       private:
        std::queue<blockchain::Transaction>* cd_txq_;
        std::mutex* cd_io_mutex_;
        std::mutex* cd_txq_mutex_;

        chat::NewTransaction request_;
        chat::Ack reply_;
        ServerAsyncResponseWriter<chat::Ack> responder_;

        std::condition_variable* cd_cv_ = nullptr;
    };

    class CallDataSync : public CallData {
       public:
        // One Calldata object handling each request thread
        CallDataSync(Chat::AsyncService* service, ServerCompletionQueue* cq, std::unordered_map<uint32_t, blockchain::UTXOlist>* utxolists,
                     std::mutex* io_mutex, std::mutex* utxolists_mutex);

        void Proceed() override;

       private:
        std::unordered_map<uint32_t, blockchain::UTXOlist>* cd_utxolists_;
        std::mutex* cd_utxolists_mutex_;
        std::mutex* cd_io_mutex_;

        chat::SyncReq request_;
        chat::SyncReply reply_;
        ServerAsyncResponseWriter<chat::SyncReply> responder_;
    };

   private:
    void HandleRpcs();
    Chat::AsyncService service_;
    std::unique_ptr<Server> server_;
    std::unique_ptr<ServerCompletionQueue> cq_;
    std::atomic<bool> is_running_{true};

    std::queue<blockchain::Message> mq_;       // consensus message queue
    std::queue<blockchain::Transaction> txq_;  // new transactions queue
    std::mutex mq_mutex_;
    std::mutex txq_mutex_;
    std::mutex io_mutex_;
    std::unordered_map<uint32_t, blockchain::UTXOlist> utxolists_;
    std::mutex utxolists_mutex_;

    std::condition_variable cv_;
};
}  // namespace comms

#endif
