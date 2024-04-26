#include "server.h"

using namespace comms;

ChatServiceImpl::~ChatServiceImpl()  // destructor, this is executed when the object gets out of scope
{
    server_->Shutdown();
    cq_->Shutdown();
}

void ChatServiceImpl::Run(uint16_t port) {  // configures the server to listen on a specified port and starts the server.
    std::string server_address = absl::StrFormat("0.0.0.0:%d", port);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);

    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

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

// One Calldata object handling each request thread
ChatServiceImpl::CallData::CallData(Chat::AsyncService *service, ServerCompletionQueue *cq, std::queue<blockchain::Message> *mq,
                                    std::mutex *io_mutex, std::mutex *mq_mutex)
    : cd_service_(service), cd_cq_(cq), responder_(&ctx_), status_(CREATE), cd_io_mutex_(io_mutex), cd_mq_mutex_(mq_mutex), cd_mq_(mq) {
    Proceed();
}

// Modified statuses to handle concurrent requests coming to cq_.
void ChatServiceImpl::CallData::Proceed() {
    if (status_ == CREATE) {
        status_ = START_PROCESS;
        cd_service_->RequestTalk(&ctx_, &request_, &responder_, cd_cq_, cd_cq_,
                                 this);
        //std::cout << "CREATE" << std::endl;
    } else if (status_ == START_PROCESS) {
        new CallData(cd_service_, cd_cq_, cd_mq_, cd_io_mutex_, cd_mq_mutex_);

        // The actual processing.
        blockchain::Message message_(request_.message());

        // prevent from multiple server threads writing to
        //cd_io_mutex_->lock();
        //std::cout << "Received from " << message_.getSenderId() << " transaction: " << message_.getBlock().getTransaction() << std::endl;
        // cd_io_mutex_->unlock();

        cd_mq_mutex_->lock();
        cd_mq_->push(message_);
        cd_mq_mutex_->unlock();
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
    new CallData(&service_, cq_.get(), &mq_, &io_mutex_, &mq_mutex_);

    void *tag;
    bool ok;

    while (true) {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        static_cast<CallData *>(tag)->Proceed();
    }
}
