#include "comms/client.h"

using namespace comms;

// -------------------------------- ChatClient --------------------------------

void ChatClient::Talk(const blockchain::Message &message) {
    // Data we are sending to the server.
    Send request;
    chat::Message msg = message.toProtoMessage();
    // request.set_allocated_message(&msg);
    *request.mutable_message() = msg;

    // Container for the data we expect from the server.
    Ack reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Talk(&context, request, &reply);

    // std::cout << message.getBlock().getTransaction() << std::endl;
    // Act upon its status.
    /*if (status.ok()) {
        std::cout << "Received ack" << std::endl;
    } else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        std::cout << "RPC failed" << std::endl;
    }*/
}

// -------------------------------- ChatClientImpl --------------------------------

int ChatClientImpl::ReadServerFile_(const std::string filepath) {
    // open file
    std::fstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return 1;  // Return an error code
    }

    // Read each line from the file and store it in the vector
    std::string line;
    while (std::getline(file, line)) {
        servers_.push_back(line);
    }

    // Close the file
    file.close();

    return 0;
}

int ChatClientImpl::SendMessage_(const blockchain::Message message, const std::string server_address) {
    // io_mtx_.lock();
    // std::cout << "Thread (" << server_address << ") sending message: " << message.getBlock().getTransaction() << std::endl;
    // io_mtx_.unlock();

    ChatClient chat(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    chat.Talk(message);

    return 0;
}

int ChatClientImpl::Broadcast(const blockchain::Message message) {
    // Lock queues
    {
        std::lock_guard<std::mutex> cv_lock(mq_mtx_);
        // Add message to every que
        for (int i = 0; i < message_queues_.size(); ++i) {
            message_queues_[i]->push(message);
        }
    }
    // Notify threads a new message is added & release lock
    mq_cv_.notify_all();

    // std::cout << "Broadcast Successfully!" << std::endl;

    return 0;
}

int ChatClientImpl::SendToLeader(const blockchain::Message message) {
    const std::string leader_address = "192.168.0.5:50000";
    this->SendMessage_(message, leader_address);

    return 0;
}

void ChatClientImpl::RunThread_(const std::string server_address, std::shared_ptr<std::queue<blockchain::Message>> mq) {
    // io_mtx_.lock();
    // std::cout << "Thread started, server address: " << server_address << std::endl;
    // io_mtx_.unlock();
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

ChatClientImpl::ChatClientImpl() {
    // Read server addresses from file
    ReadServerFile_("servers.txt");

    uint8_t num_servers = servers_.size();
    message_queues_.reserve(num_servers);
    stop_threads_ = false;

    // Start RunThread with different server & queue
    for (int i = 0; i < num_servers; ++i) {
        std::shared_ptr<std::queue<blockchain::Message>> thread_queue = std::make_shared<std::queue<blockchain::Message>>();
        message_queues_.push_back(thread_queue);
        thread_pool_.push_back(std::thread([&, i, thread_queue]() { RunThread_(servers_[i], thread_queue); }));
    }
}

ChatClientImpl::~ChatClientImpl() {
    st_mtx_.lock();
    stop_threads_ = true;
    st_mtx_.unlock();
    mq_cv_.notify_all();
    for (int i = 0; i < thread_pool_.size(); ++i) {
        thread_pool_[i].join();
    }
}
