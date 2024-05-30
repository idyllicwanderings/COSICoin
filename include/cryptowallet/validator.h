#ifndef COSICOIN_VALIDATOR_H
#define COSICOIN_VALIDATOR_H

#include "blockchain/block.h"
#include "blockchain/transaction.h"
#include "blockchain/utxo.h"
#include "comms/client.h"
#include "comms/server.h"
#include "signature/hash.h"
#include "bracha/node.h"
#include "json/jsonparser.h"
#include "database/database.h"


#include <cstdio>
#include <iostream>
#include <chrono>
#include <algorithm>
// #define NDEBUG
#include <cassert>

#include <mutex>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <vector>
#include <unistd.h>
#include <functional>


namespace cryptowallet {


// Represents different types of Event.
enum class Event {
    NONE = 0,
    SYNC = 1 << 0,
    TX = 1 << 1,
    PROPOSE = 1 << 2,
    DBSTORE = 1 << 3
};


/**
 * @note This class is used to loop and execute any registered events for a validator instance.
*/
class EventHandler {
    
public:
    typedef std::function<void()> HandleEvent;
    //typedef void(*HandleEvent)(/*void*, void**/);

    // function call
    struct EventSubscriberNode {
        void* _this;
        HandleEvent func;
    };
    
    // Event type, data unused
    struct EventPublishNode {
        Event event;
        void* data;
    };

public:

    EventHandler() {
        std::unique_lock<std::mutex> ulock(_mutexSubscriber, std::defer_lock);
        _sub_ulock = std::move(ulock);

        std::unique_lock<std::mutex> plock(_mutexPublish, std::defer_lock);
        _pub_ulock = std::move(plock);

        
        th = std::thread(std::bind(EventHandler::EventProcess, this));
        pt_id = th.get_id();
        //t.detach();
    }
    
    // forbids copy constructor.
    EventHandler(const EventHandler&) = delete;
    
    ~EventHandler() {
        //if (th.joinable()) th.join();
    }
    
    /**
     * @brief subscribe to a event
    */
    void SubscriberEvent(Event event, EventSubscriberNode node) {
        _pub_ulock.lock();
        subsMap_[event].push_back(node);
        _pub_ulock.unlock();
    }

    /**
     * @brief unsubscribe the event 
     * @note deprecated because of == operator not overloaded
    */
    void releaseSubscriberEvent(Event event, EventSubscriberNode node) {
        _sub_ulock.lock();
        auto it = subsMap_.find(event);
        if(it != subsMap_.end()) {
            for(auto ite = it->second.begin(); ite != it->second.end(); ) {
                if(ite->_this == node._this /*&& ite->func == node.func*/) {
                    ite = it->second.erase(ite);
                }
                else {
                    ++ite;
                }
            }
        }
        _sub_ulock.unlock();
    }


    /**
     * @brief publish the event.
    */
    void PublishEvent(Event event/*, void* data*/) {
        EventPublishNode node;
        node.event = event;
        // node.data = data;
        
        _pub_ulock.lock();
        publist.push_back(node);
        _pub_ulock.unlock();
    }

    /**
     * @brief the event handler thread processing all registered events.
    */
    static void* EventProcess(void* _this) {

        EventHandler* th = (EventHandler*)_this;
        while (1)
        {
            EventPublishNode cur {Event::NONE, nullptr};

            th->_mutexPublish.lock();
            if (th->publist.empty()) {
                th->_mutexPublish.unlock();
                continue;
            }

            cur = th->publist.front();
            th->publist.pop_front();
            th->_mutexPublish.unlock();
            
            // pop and execute all the registered events of the same event type.
            th->_mutexSubscriber.lock();
            auto it = th->subsMap_.find(cur.event);
            if(it != th->subsMap_.end()) {
                for(auto ite = it->second.begin(); ite != it->second.end(); ++ite) {
                    //ite->func(ite->_this, cur.data);
                    // std::cout << "EventHandler.cc: " << "EventProcess(): " << "running the subscribed task..." << std::endl;
                    ite->func();
                }
            }

            th->_mutexSubscriber.unlock();
        }
    }

    /**
     * @brief Add new events to handle.
    */
    template <typename T>
    void runInLoop(/*void* ber, */
                   Event event,
                   T&& task) {
        PublishEvent(event);
        EventHandler::EventSubscriberNode node;
        node.func = task;
        
        SubscriberEvent(event, node);
    }


    public:
        std::mutex _mutexPublish;
        std::unique_lock<std::mutex> _pub_ulock;

        std::mutex _mutexSubscriber;
        std::unique_lock<std::mutex> _sub_ulock;
        
        std::thread th;
        std::thread::id pt_id;

        std::list<EventPublishNode> publist;
        std::map<Event, std::list<EventSubscriberNode>> subsMap_;
};



class Validator {
   public:
    explicit Validator(
              uint32_t id,
              config::Settings settings,
              comms::ChatServiceImpl* grpcServer,
              bracha::Node* node,
              const std::vector<uint32_t>& wids) 
              : id_(id), 
                wallets_ids_(wids), 
                _grpcServer(grpcServer), 
                node_(node), 
                _evecenter(new EventHandler()),
                my_pk_(signature::Signature::getInstance()) 
    { 
        _mutex_grpc = new std::mutex();
        port_ = settings.getValidatorInfo(id).port;
        ipaddr_ = settings.getValidatorInfo(id).address;
        _cond_grpc = _grpcServer->getConditionVariable();
        db_ = new database::Database("../../database"+ std::to_string(id_)+ ".json");
        if (node->isLeader()) {
            node = dynamic_cast<bracha::LeaderNode*>(node); 
        }      
    }

    ~Validator() {
        delete _mutex_grpc;
    }
    
    // Warning: only supports move construction and no copy construction due to mutex.
    Validator(Validator&& v):id_(v.id_),
                             ipaddr_(v.ipaddr_),
                             port_(v.port_),
                             wallets_ids_(v.wallets_ids_),
                             _grpcServer(std::move(v._grpcServer)),
                             node_(std::move(v.node_)),
                             _evecenter(v._evecenter),
                             _cond_grpc(v._cond_grpc),
                             _mutex_grpc(v._mutex_grpc),
                             _listen(v._listen),
                             _consensus(v._consensus),
                             _propose(v._propose),
                             blk_intervals_(v.blk_intervals_),
                             my_pk_(std::move(v.my_pk_)),
                             pk_sets_(v.pk_sets_),
                             utxo_sets_(v.utxo_sets_),
                             memory_pool_(v.memory_pool_),
                             blk_verify_pks_(v.blk_verify_pks_),
                             db_(v.db_),
                             blk_hashes_(v.blk_hashes_)
    {
        v._evecenter = nullptr;
        v._cond_grpc = nullptr;
        v._mutex_grpc = nullptr;
        v._grpcServer = nullptr;
        v.node_ = nullptr;
        v.db_ = nullptr;
        
        //free(my_pk_);
    }

    // @attention: does not allow Copy, do std::move
    Validator(Validator&) = delete;

    /**
     * @brief Running the threads separately.
    */
    void Run() {
        _listen_th = std::thread(std::bind(&Validator::Listen, this));
        if (node_->isLeader()) _blkpropose_th = std::thread(std::bind(&Validator::ProposeProcess, this));
        _consensus_th = std::thread(std::bind(&Validator::ConsensusProcess, this));
        _DBstore_th = std::thread(std::bind(&Validator::DBProcess, this));
    }
    
    /**
     * @brief distribute a certain amount of coins equally to every wallet.
    */
    void InitializeCoins(uint64_t coins);
    
    /**
     * @brief Ouputs the blockchain to the specified file.
    */
    void SaveBlockchain(std::string filename){
        db_->save(filename);
    }

    inline constexpr uint32_t getId() const { return id_; }

   private:
    /**
     * @brief update the current public keys according to the newly received Tx.
     * @param sender_id, transaction
     */
    void UpdateKeys(uint32_t sender_id, const blockchain::Transaction& tx);
    
    /**
     * @brief passes on handling of a newly arrived txlist
    */
    int OnTxReply(std::vector<blockchain::Transaction>& txlist);

    /**
     * @brief Upon receiving a new Transaction, 
     * the validator updates the corresponding public key despite the
     * possible invalidity of signature; then it checks the signature 
     * of the transaction using its stored public key; it proceeds to 
     * checks whether the output(h,i) is in the local UTXO set or the 
     * spending conditions are met; finally it adds the current 
     * transactions to the memory pool.
     *
     * @param a transaction Tx
     * @return 0 a Tx is invalid and discarded
     * @return 1 a Tx is received
     */
    int OnRecvTx(blockchain::Transaction& tx);

    /**
     * @brief creates a new block:
     * the leader validator first takes the newest transaction available 
     * from the memory pool, and first check its validity and other 
     * conditions; then the leader checks whether the current transaction
     * is consistent with local UTXO sets; then it also check whether the
     * transaction is consistent with the proposed block to prevent double
     * spending. If all the checks pass, then the validator calls on the 
     * consensus protocol to propose a new block.
     *
     * @param a Block blk
     * @return 1 success
     * @return 0 fail
     */
    int CreateBlk(blockchain::Block& bx);
    
    
    /**
     * @brief after obtaining an accepted block, the validator updates the
     *  UTXOs and adds the block to the chain.
     *
     * @param a Block blk
     * @return 1 success
     * @return 0 fail
     */
    int OnAgreeBlk(blockchain::Block& bx);

    /**
     * @warning : previously serves Synchronization Callback function to the server,
     *            now deprecated due to server's interfaces' change.
     *
     * @brief: Synchronization (no longer as Callback) function to the server
     *
     * @param a transaction Tx
     * @return 1 changes added to the wallet's coins
     * @return 0 no changes are added to the wallet
     */
    int OnSyncReply(const std::vector<uint32_t>& idlist);

    /**
     * @brief Calling consensus to propose the new block
     * @return 1 success
     * @return 0 fail
    */
    int Propose(blockchain::Block& blk) const;


    // Seriously, this should be named requestcallback.
    template <typename T>
    using ReplyCallback = std::function<int(const std::vector<T>&)>;

    // request call functions like getMessages, getTX, getSyncs
    template <typename T>
    using RequestCall = std::function<bool(const std::vector<T>&)>;

    /**
     * @brief handles newly arrived request and send reply to gRPC servers.
     *
     * @param T: the class, transaction or uint32_t
     *        req: the request call function
     *        list: the arguments for request call function
     *        event: sync or transaction
     *        done: request callback function
     */
    template <typename T>
    void OnRequest(const RequestCall<T>& req,
                              const std::vector<T>& list,
                              Event event,
                              const ReplyCallback<T>& done);
    /**
     * @brief polling and listening for new requests from gRPC interfaces
    */
    int Listen();
    
    /**
     * @brief intersection of two containers
     */ 
    template <typename T>
    static std::vector<T> vectors_intersection(const std::vector<T>& v1, 
                                               const std::vector<T>& v2)
    {
        std::vector<T> v;
        sort(v1.begin(),v1.end());   
        sort(v2.begin(),v2.end());   
        set_intersection(v1.begin(),v1.end(),v2.begin(),v2.end(),back_inserter(v)); 
	    return v;
    }

   private:

    /**
     * @brief Thread executing consensus
    */
    void ConsensusProcess(void);

    /**
     * @brief Thread creating events for proposing new blocks
    */
    void ProposeProcess(void);

    /**
     * @brief Thread detecting any new accepted blocks and do after-agreement operations.
    */
    void DBProcess(void);


    /**
     * @brief Register a new event to the event handler.
     *        perfect forward any rvalues if needed : can be very slow.
     * @param Event: event
     *        task: functional call
    */
    template <typename T>
    void RunTaskInLoop(Event event, T&& task)
    {
        _evecenter->runInLoop(/**this,*/ event, std::forward<T>(task));
    }

   private:
    /**
     * @brief thread handler and timing parameters
    */

    // one thread in Event Handler for customized events including CreateBlk, RecvTx, etc..
    EventHandler* _evecenter;
    std::condition_variable* _cond_grpc;
    std::mutex* _mutex_grpc;
    
   public:
    // one thread for listening from gRPC server
    std::thread _listen_th;
     // one thread for running consensus only
    std::thread _consensus_th;
    // one thread for constantly proposing a block
    std::thread _blkpropose_th;
    std::thread _DBstore_th;
   
   private:
    // running control bool for the above threads
    bool _listen = true;
    bool _consensus = true;
    bool _propose = true; 
    bool _DBstore = true;

    const uint32_t blk_intervals_ = 20;

    /**
     * configuration settings
    */
    const uint32_t id_;
    std::string ipaddr_;
    int port_;

    /**
     * gRPC instances
    */
    comms::ChatServiceImpl* _grpcServer;
    
    /**
     * Node instance
    */
    bracha::Node* node_;
    
    /**
     * wallets info
    */
    std::vector<uint32_t> wallets_ids_;

    signature::Signature my_pk_;

    std::unordered_map<uint32_t, signature::SigKey> pk_sets_;

  public:
  // @warning UNCOMMENT for real use
  //private:
    std::unordered_map<uint32_t, blockchain::UTXOlist> utxo_sets_;
    std::vector<blockchain::Transaction> memory_pool_;
    std::unordered_map<std::string, signature::SigKey> blk_verify_pks_;
    
    std::vector<blockchain::Block> blist_;
    
    /**
     * Database
    */
    // leveldb::DB* blockchain_DB_;
    // @warning: should be query to the database instead of handling a blk hash list
    std::vector<std::string> blk_hashes_; 
    database::Database* db_;
};

}  // namespace cryptowallet

#endif
