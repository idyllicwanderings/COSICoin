#include "comms/server.h"

using namespace comms;

ChatServiceImpl::~ChatServiceImpl() {  // destructor, this is executed when the object gets out of scope
    is_running_ = false;
    server_->Shutdown();
    cq_->Shutdown();
}

void ChatServiceImpl::Run(uint16_t port) {  // configures the server to listen on a specified port and starts the server.
    std::string server_address = "0.0.0.0:" + std::to_string(port);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);

    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    io_mutex_.lock();
    std::cout << "Server listening on " << server_address << std::endl;
    io_mutex_.unlock();

    is_running_ = true;
    HandleRpcs();
}

bool ChatServiceImpl::getMessages(std::vector<blockchain::Message> &msg_list) {
    mq_mutex_.lock();
    if (mq_.empty()) {
        mq_mutex_.unlock();
        return false;
    } else {
        while (!mq_.empty()) {
            msg_list.push_back(mq_.front());
            mq_.pop();
        }
        mq_mutex_.unlock();
        return true;
    }
}

bool ChatServiceImpl::newConsensusMessages() {
    std::lock_guard<std::mutex> lock_m(mq_mutex_);
    return !mq_.empty();
}

bool ChatServiceImpl::newRequests() {
    /*io_mutex_.lock();
    std::cout << "Server.cc:" << "newRequests in " << std::endl;
    io_mutex_.unlock();*/
    std::lock_guard<std::mutex> lock_tx(txq_mutex_);
    return !txq_.empty();
}

bool ChatServiceImpl::getTransactions(std::vector<blockchain::Transaction> &tx_list) {
    txq_mutex_.lock();
    if (txq_.empty()) {
        txq_mutex_.unlock();
        return false;
    } else {
        while (!txq_.empty()) {
            tx_list.push_back(txq_.front());
            txq_.pop();
        }
        txq_mutex_.unlock();
        return true;
    }
}

void ChatServiceImpl::setUTXOlists(std::unordered_map<uint32_t, blockchain::UTXOlist> utxolists) {
    utxolists_mutex_.lock();
    utxolists_.clear();
    utxolists_ = utxolists;
    utxolists_mutex_.unlock();
}

// One Calldata object handling each request thread
ChatServiceImpl::CallDataTalk::CallDataTalk(Chat::AsyncService *service, ServerCompletionQueue *cq, std::queue<blockchain::Message> *mq,
                                            std::mutex *io_mutex, std::mutex *mq_mutex, std::condition_variable *cv)
    : CallData(service, cq), responder_(&ctx_), cd_io_mutex_(io_mutex), cd_mq_mutex_(mq_mutex), cd_mq_(mq), cd_cv_(cv) {
    Proceed();
}

// Modified statuses to handle concurrent requests coming to cq_.
void ChatServiceImpl::CallDataTalk::Proceed() {
    if (status_ == CREATE) {
        status_ = START_PROCESS;
        cd_service_->RequestTalk(&ctx_, &request_, &responder_, cd_cq_, cd_cq_,
                                 this);
        // std::cout << "CREATE" << std::endl;
    } else if (status_ == START_PROCESS) {
        new CallDataTalk(cd_service_, cd_cq_, cd_mq_, cd_io_mutex_, cd_mq_mutex_, cd_cv_);

        // The actual processing.
        blockchain::Message message_(request_.message());

        cd_io_mutex_->lock();
        std::cout << "Server: received message from " << message_.getSenderId() << std::endl;
        cd_io_mutex_->unlock();

        cd_mq_mutex_->lock();
        cd_mq_->push(message_);
        cd_mq_mutex_->unlock();

        // Notify cv
        if (cd_cv_ != nullptr) {
            cd_cv_->notify_all();
        }

        // Send ack reply
        responder_.Finish(reply_, Status::OK, this);
        status_ = FINISH;

        // std::cout << "FINISH" << std::endl;
    } else {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

// One Calldata object handling each request thread
ChatServiceImpl::CallDataNewTx::CallDataNewTx(Chat::AsyncService *service, ServerCompletionQueue *cq, std::queue<blockchain::Transaction> *txq,
                                              std::mutex *io_mutex, std::mutex *txq_mutex, std::condition_variable *cv)
    : CallData(service, cq), responder_(&ctx_), cd_io_mutex_(io_mutex), cd_txq_mutex_(txq_mutex), cd_txq_(txq), cd_cv_(cv) {
    Proceed();
}

// Modified statuses to handle concurrent requests coming to cq_.
void ChatServiceImpl::CallDataNewTx::Proceed() {
    if (status_ == CREATE) {
        status_ = START_PROCESS;
        cd_service_->RequestNewTx(&ctx_, &request_, &responder_, cd_cq_, cd_cq_,
                                  this);
        // std::cout << "CREATE" << std::endl;
    } else if (status_ == START_PROCESS) {
        new CallDataNewTx(cd_service_, cd_cq_, cd_txq_, cd_io_mutex_, cd_txq_mutex_, cd_cv_);

        // The actual processing.
        blockchain::Transaction transaction_;
        transaction_.fromProtoTransaction(request_.transaction());

        cd_io_mutex_->lock();
        std::cout << "Server: received new transaction from " << transaction_.getSenderID() << std::endl;
        cd_io_mutex_->unlock();

        cd_txq_mutex_->lock();
        cd_txq_->push(transaction_);
        cd_txq_mutex_->unlock();

        // Notify cv
        if (cd_cv_ != nullptr) {
            cd_cv_->notify_all();
        }

        // Send ack reply
        responder_.Finish(reply_, Status::OK, this);
        status_ = FINISH;

        // std::cout << "FINISH" << std::endl;
    } else {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

// One Calldata object handling each request thread
ChatServiceImpl::CallDataSync::CallDataSync(Chat::AsyncService *service, ServerCompletionQueue *cq, std::unordered_map<uint32_t, blockchain::UTXOlist> *utxolists,
                                            std::mutex *io_mutex, std::mutex *utxolists_mutex)
    : CallData(service, cq), responder_(&ctx_), cd_io_mutex_(io_mutex), cd_utxolists_(utxolists), cd_utxolists_mutex_(utxolists_mutex) {
    Proceed();
}

// Modified statuses to handle concurrent requests coming to cq_.
void ChatServiceImpl::CallDataSync::Proceed() {
    if (status_ == CREATE) {
        status_ = START_PROCESS;
        cd_service_->RequestSync(&ctx_, &request_, &responder_, cd_cq_, cd_cq_,
                                 this);
        // std::cout << "CREATE" << std::endl;
    } else if (status_ == START_PROCESS) {
        new CallDataSync(cd_service_, cd_cq_, cd_utxolists_, cd_io_mutex_, cd_utxolists_mutex_);

        // The actual processing.
        uint32_t wallet_id = request_.wallet_id();

        cd_io_mutex_->lock();
        std::cout << "Server: received sync request from wallet " << wallet_id << std::endl;
        cd_io_mutex_->unlock();

        // Get utxolist for wallet id
        blockchain::UTXOlist utxolist;
        if (cd_utxolists_->find(wallet_id) != cd_utxolists_->end()) {
            utxolist = (*cd_utxolists_)[wallet_id];

            cd_io_mutex_->lock();
            std::cout << "Server: sending utxolist back to wallet " << wallet_id << std::endl;
            cd_io_mutex_->unlock();
        } else {
            cd_io_mutex_->lock();
            std::cout << "Server: no utxolist found, sending empty utxolist back to wallet " << wallet_id << std::endl;
            cd_io_mutex_->unlock();
        }

        // Add utxolist to reply
        chat::UTXOlist proto_utxolist;
        utxolist.toProtoUTXOlist(&proto_utxolist);
        *reply_.mutable_utxolist() = proto_utxolist;

        responder_.Finish(reply_, Status::OK, this);
        status_ = FINISH;

        // std::cout << "FINISH" << std::endl;
    } else {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

void ChatServiceImpl::HandleRpcs()  // This method initiates the handling of incoming RPCs.
// It continuously waits for incoming requests and handles them asynchronously.
{
    new CallDataTalk(&service_, cq_.get(), &mq_, &io_mutex_, &mq_mutex_, &cv_);
    new CallDataNewTx(&service_, cq_.get(), &txq_, &io_mutex_, &txq_mutex_, &cv_);
    new CallDataSync(&service_, cq_.get(), &utxolists_, &io_mutex_, &utxolists_mutex_);

    CallData *tag;
    bool ok;

    while (is_running_) {
        GPR_ASSERT(cq_->Next(reinterpret_cast<void **>(&tag), &ok));
        GPR_ASSERT(ok);
        // Call Proceed on the correct type of CallData
        if (dynamic_cast<CallDataTalk *>(tag)) {
            static_cast<CallDataTalk *>(tag)->Proceed();
        } else if (dynamic_cast<CallDataNewTx *>(tag)) {
            static_cast<CallDataNewTx *>(tag)->Proceed();
        } else if (dynamic_cast<CallDataSync *>(tag)) {
            static_cast<CallDataSync *>(tag)->Proceed();
        }
    }
}

void ChatServiceImpl::clearUTXOlists() {
    utxolists_mutex_.lock();
    utxolists_.clear();
    utxolists_mutex_.unlock();
}
