#include "comms/client.h"

using namespace comms;

// -------------------------------- ChatClient --------------------------------

void ChatClient::Talk(const blockchain::Message &message) {
    // Data we are sending to the server.
    chat::Send request;
    chat::Message msg = message.toProtoMessage();
    // request.set_allocated_message(&msg);
    *request.mutable_message() = msg;

    // Container for the data we expect from the server.
    chat::Ack reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Talk(&context, request, &reply);
}

void ChatClient::Sync(const uint32_t wallet_id, blockchain::UTXOlist *utxolist) {
    chat::SyncReq request;
    request.set_wallet_id(wallet_id);

    chat::SyncReply reply;

    ClientContext context;

    Status status = stub_->Sync(&context, request, &reply);

    utxolist->fromProtoUTXOlist(reply.utxolist());
}

void ChatClient::NewTx(const blockchain::Transaction &transaction) {
    chat::NewTransaction request;
    chat::Transaction proto_tx;
    transaction.toProtoTransaction(&proto_tx);
    *request.mutable_transaction() = proto_tx;
    chat::Ack reply;
    ClientContext context;
    Status status = stub_->NewTx(&context, request, &reply);
}

// -------------------------------- WalletClientImpl -----------------------------------

int WalletClientImpl::SendToLeader(const blockchain::Transaction transaction) {
    ChatClient chat(grpc::CreateChannel(leader_, grpc::InsecureChannelCredentials()));
    chat.NewTx(transaction);

    return 1;
}

int WalletClientImpl::SendSyncRequest(const uint32_t wallet_id, blockchain::UTXOlist &utxolist) {
    ChatClient chat(grpc::CreateChannel(leader_, grpc::InsecureChannelCredentials()));
    chat.Sync(wallet_id, &utxolist);

    return 1;
}

WalletClientImpl::WalletClientImpl(config::Settings settings) {
    // Read leader addres from file
    leader_ = settings.getLeaderAddress();
}

// -------------------------------- ValidatorClientImpl --------------------------------

int ValidatorClientImpl::SendMessage_(const blockchain::Message message, const std::string server_address) {
    std::cout << "Client:: SendMessage_ to " << server_address << std::endl;
    ChatClient chat(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    chat.Talk(message);

    return 1;
}

int ValidatorClientImpl::Broadcast(const blockchain::Message message) {
    // Lock queues
    {
        std::lock_guard<std::mutex> cv_lock(mq_mtx_);
        // Add message to every que
        for (int i = 0; i < servers_.size(); ++i) {
            message_queues_[servers_[i]]->push(message);
        }
    }
    // Notify threads a new message is added & release lock
    mq_cv_.notify_all();

    // std::cout << "Broadcast Successfully!" << std::endl;

    return 1;
}

int ValidatorClientImpl::SendToLeader(const blockchain::Message message) {
    // this->SendMessage_(message, leader_);
    mq_mtx_.lock();
    message_queues_[leader_]->push(message);
    mq_mtx_.unlock();
    mq_cv_.notify_all();

    return 1;
}

void ValidatorClientImpl::RunThread_(const std::string server_address, std::shared_ptr<std::queue<blockchain::Message>> mq) {
    st_mtx_.lock();
    while (!stop_threads_) {
        st_mtx_.unlock();

        // Wait for new message in queue
        std::unique_lock<std::mutex> cv_lock(mq_mtx_);
        mq_cv_.wait(cv_lock, [&mq, this] { return !mq->empty() || stop_threads_; });

        if (!mq->empty()) {
            // Get & delete message from queue
            blockchain::Message msg = mq->front();
            mq->pop();
            mq_mtx_.unlock();

            // Send message
            SendMessage_(msg, server_address);
        }

        st_mtx_.lock();
    }
    st_mtx_.unlock();
}

ValidatorClientImpl::ValidatorClientImpl(config::Settings settings) {
    // Read server addresses from file
    servers_ = settings.getValidatorAdresses();
    leader_ = settings.getLeaderAddress();

    startThreads_();
}

void ValidatorClientImpl::startThreads_() {
    uint8_t num_servers = servers_.size();
    stop_threads_ = false;

    // Start RunThread with different server & queue
    for (int i = 0; i < num_servers; ++i) {
        std::shared_ptr<std::queue<blockchain::Message>> thread_queue = std::make_shared<std::queue<blockchain::Message>>();
        message_queues_[servers_[i]] = thread_queue;
        thread_pool_.push_back(std::thread([&, i, thread_queue]() { RunThread_(servers_[i], thread_queue); }));
    }
}

ValidatorClientImpl::~ValidatorClientImpl() {
    st_mtx_.lock();
    stop_threads_ = true;
    st_mtx_.unlock();
    mq_cv_.notify_all();
    for (int i = 0; i < thread_pool_.size(); ++i) {
        thread_pool_[i].join();
    }
}
