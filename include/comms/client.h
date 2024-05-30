#ifndef COSICOIN_COMMS_CLIENT_H
#define COSICOIN_COMMS_CLIENT_H

#include <chat.grpc.pb.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <condition_variable>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "blockchain/message.h"
#include "blockchain/transaction.h"
#include "blockchain/utxo.h"
#include "config/settings.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using chat::Chat;

namespace comms {

class WalletClientImpl {
   public:
    WalletClientImpl(){};
    WalletClientImpl(config::Settings settings);
    // Copy constructor
    WalletClientImpl(const WalletClientImpl &wallet_client) : leader_(wallet_client.leader_){};
    // Move constructor
    WalletClientImpl(WalletClientImpl &&wallet_client) noexcept : leader_(std::move(wallet_client.leader_)){};

    /*
     * Sends the specified transaction to the leader validator
     */
    int SendToLeader(const blockchain::Transaction transaction);

    /*
     * Sends a sync request to the leader
     * Fills utxolist with the received answer
     */
    int SendSyncRequest(const uint32_t wallet_id, blockchain::UTXOlist &utxolist);

   private:
    // Address of the leader
    std::string leader_;
};

class ValidatorClientImpl {
   public:
    ValidatorClientImpl(){};
    ValidatorClientImpl(config::Settings settings);
    // Copy constructor
    ValidatorClientImpl(const ValidatorClientImpl &validator_client) : servers_(validator_client.servers_), leader_(validator_client.leader_) { startThreads_(); };
    // Move constructor
    ValidatorClientImpl(ValidatorClientImpl &&validator_client) noexcept : servers_(std::move(validator_client.servers_)), leader_(std::move(validator_client.leader_)) { startThreads_(); };

    ~ValidatorClientImpl();

    /**
     * @brief Broadcasts a message to the network.
     *
     * Sends the specified message to all configured servers.
     *
     * @param message The message to be broadcasted.
     * @return An integer indicating the status of the broadcast.
     */
    int Broadcast(const blockchain::Message message);

    /*
     * Sends the specified message to the leader validator
     */
    int SendToLeader(const blockchain::Message message);

   private:
    // Vector to keep server addresses
    std::vector<std::string> servers_;
    // Address of the leader
    std::string leader_;

    // Queues of messages that need to be send to the servers
    // One queue per server
    std::map<std::string, std::shared_ptr<std::queue<blockchain::Message>>> message_queues_;

    // Condition variable to notify threads a new message is added to the queues
    std::condition_variable mq_cv_;

    // Mutex for the queues
    std::mutex mq_mtx_;

    // Thread pool for sending threads
    std::vector<std::thread> thread_pool_;

    bool stop_threads_;
    std::mutex st_mtx_;

    // Run one thread for one server and give it it's queue
    void RunThread_(const std::string server_address, std::shared_ptr<std::queue<blockchain::Message>> mq);

    // Send message to a specific server
    int SendMessage_(const blockchain::Message message, const std::string server_address);

    void startThreads_();
};

// Class to handle the GRPC client itself
class ChatClient {
   public:
    explicit ChatClient(std::shared_ptr<Channel> channel) : stub_(Chat::NewStub(channel)){};

    void Talk(const blockchain::Message &message);

    void Sync(const uint32_t wallet_id, blockchain::UTXOlist *utxolist);

    void NewTx(const blockchain::Transaction &transaction);

   private:
    std::unique_ptr<Chat::Stub> stub_;
    CompletionQueue cq_;
};
}  // namespace comms

#endif