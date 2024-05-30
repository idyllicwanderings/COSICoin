#include"cryptowallet/validator.h"


using namespace cryptowallet;
using namespace blockchain;

using validator = cryptowallet::Validator;
using transaction = blockchain::Transaction;
using block = blockchain::Block;


void validator::InitializeCoins(uint64_t coins) 
{
    std::cout << "initializing coins"  << std::endl;
    
    uint32_t initTX_id = 0;
    blockchain::UTXOlist init_utxolist;
    
    for (const auto id: wallets_ids_)
    {
        blockchain::UTXO utxo(initTX_id++,0, Output(coins, id));
        utxo_sets_[id].add(utxo);
        init_utxolist.add(utxo);
    }

    _mutex_grpc->lock();
    _grpcServer->setUTXOlists(utxo_sets_);
    _mutex_grpc->unlock();

    node_->setUTXOlists(init_utxolist, wallets_ids_);
}

void validator::UpdateKeys(uint32_t sender_id, const blockchain::Transaction& tx) {
    pk_sets_[sender_id] = tx.getPublicKey();
}


int validator::OnTxReply(std::vector<transaction>& txlist) {
    std::cout << "Validator.cc: " << "OnTxReply(): " << "iterating new txs of size "  << txlist.size() << std::endl;
  
    for (auto& tx: txlist) {
        OnRecvTx(tx);
    }
}

int validator::OnRecvTx(transaction& tx) {

    std::cout << "Validator.cc: " << "OnRecvTx(): " << "new tx received..." << std::endl;

    uint16_t wallet_id = tx.getSenderID();

    // updates the PK set < id_i,PK_{n+1}> before verifying.
    // Notes: the PK gets updated despite the invalidity of signature.
    UpdateKeys(wallet_id, tx); 
    std::cout << "Validator.cc: " << "OnRecvTx(): " << "key pairs updated..." << std::endl;
    
    // checks the signature of the Sign(tx) using PK_{i,n}
    int valid = signature::Verify(tx.getDigest(), 
                                tx.getSenderSig(),
                                pk_sets_[wallet_id]);
                                
     std::cout << "Validator.cc: " << "OnRecvTx(): " << "new tx verified (can be invalid)..." << std::endl;

    /* checks whether 
     * 1) the output(h,i) is in the local UTXO set
     * 2) spending conditions(drop when not sum of inputs == sum of outputs).
     * 3) signature is valid
     */ 
    if (valid  != 1 || !tx.checkSpendingConditions(utxo_sets_[wallet_id], wallets_ids_)) {
        std::cout << "Validator.cc: " << "OnRecvTx(): " << "tx is invalid!!" <<  valid  << "  " << std::boolalpha << tx.checkSpendingConditions(utxo_sets_[wallet_id], wallets_ids_) << std::endl;
        return 0;
    }
    
    std::cout << "Validator.cc: " << "OnRecvTx(): " << "tx is valid..." << std::endl;

    // adds to the tx memory pool
    memory_pool_.push_back(tx);  
    std::cout << "Validator.cc: " << "OnRecvTx(): " << "added to memory pool..." << std::endl;
    return 1;

}


int validator::CreateBlk(Block& bx) {

    assert(/*"Only the leader can initialize and propose a new block",*/ node_->isLeader());

    // Block bx;
    std::vector<transaction> confirmed_tx;
    
    if (memory_pool_.empty()) return 0;

    while (!memory_pool_.empty()) {
        blockchain::Transaction cur_tx = memory_pool_[memory_pool_.size() - 1];
        memory_pool_.pop_back();
        std::cout << "Validator.cc: " << "CreateBlk(): " << "poped a tx" << std::endl;
        UTXOlist cur_utxo = utxo_sets_[cur_tx.getSenderID()];
        uint32_t wallet_id = cur_tx.getSenderID();
        
        // check whether the tx is consistent with local UTXO set
        if (!cur_tx.checkSpendingConditions(utxo_sets_[wallet_id], wallets_ids_)) {
            std::cout << "Validator.cc: " << "CreateBlk(): " << "the tx is discarded due to inconsistency with local UTXO set..." << std::endl;
            continue;
        }

        // the tx is consistent with the UTXOs
        /*std::for_each(confirmed_tx.begin(), confirmed_tx.end(), 
                        [cur_tx](auto t) { validator::vectors_intersection<blockchain::Output>(t.getOutputs(),
                                                            cur_tx.getOutputs())
                                                           .size() == 0; });*/                                                    
        
        // check whether the tx is consistent with the bx(double spending)
        if (!bx.verifyTxConsist(cur_tx)) {
            std::cout << "Validator.cc: " << "CreateBlk(): " << "the tx is discarded due to inconsistency with the block..." << std::endl;
            continue;
        }

        // add the current transaction to confirmed lists
        confirmed_tx.push_back(cur_tx);
        bx.addTransaction(cur_tx);

    }
    
    // finalize the block
    bx.finalize();
    
    return 1;
}



int validator::OnAgreeBlk(blockchain::Block& bx) {
    
    std::vector<blockchain::Transaction> tx_list = bx.getTransactions();

    for (auto& cur_tx: tx_list) {
        std::cout << "Validator.cc: " << "OnAgreeBlk(): " << "iterating a tx" << std::endl;
        UTXOlist cur_utxo = utxo_sets_[cur_tx.getSenderID()];
        uint32_t wallet_id = cur_tx.getSenderID();

        // update the local UTXO sets
        std::vector<blockchain::Input> inputs = cur_tx.getInputs();
        
        // delete the used UTXOs, updates with the new
        blockchain::UTXOlist ulist = utxo_sets_[cur_tx.getSenderID()];
        for (int i = 0; i < inputs.size(); i++) {
            Input cur_in = inputs[i];
            int idx = ulist.index(cur_in);
            if (idx != -1) {
               utxo_sets_[cur_tx.getSenderID()].erase(idx);
               Output cur_out = cur_tx.getOutputAt(i);
               UTXO cur_utxo = UTXO(cur_tx.getID(), i, cur_out);
            }
            else {
               std::cout << "Validator.cc: " << "CreateBlk(): " << "the tx is discarded due to inconsistency with the UTXO??..." << std::endl;
            }
        }

    }
    return 1;
}


// int validator:: OnSyncReply(const std::vector<uint32_t>& idlist) {
//    std::map<uint32_t, std::vector<UTXOlist>>& reply_utxos;

//    for (const auto& id: idlist) {
//        reply_utxos[id] = utxo_sets_[id].clone();
//    }
//    _grpcServer->setUTXOlists()
//    _grpcServer->SyncReply(idlist, reply_utxos);

// }
/**
 * @warning : arguments set only to maintain the generic callback template
*/
int validator::OnSyncReply(const std::vector<uint32_t>& idlist) {
   //std::cout << "Validator.cc: " << "OnSyncReply(): " << "new utxosets updated with the servers" <<std::endl;
   _mutex_grpc->lock();
   _grpcServer->setUTXOlists(utxo_sets_);
   _mutex_grpc->unlock();
}


int validator::Propose(blockchain::Block& blk) const
{
    assert(/*"Only the leader can propose",*/ node_->isLeader());
    // bracha::LeaderNode* node = dynamic_cast<bracha::LeaderNode*>(node_);
    // if (!node) throw std::logic_error("cannot cast node to Leadernode");
    std::cout << "validator.cc: " << "Propose(): " << "propose a new blk" << std::endl;
    return (dynamic_cast<bracha::LeaderNode*>(node_))->Propose(blk);
}


template <typename T>
void validator::OnRequest(const RequestCall<T>& req,
                          const std::vector<T>& list,
                          Event event,
                          const ReplyCallback<T>& done) 
{
    RunTaskInLoop(event, [=]() {
        if (req(list)) {
            std::cout << "Validator.cc: " << "OnRequest(): " << "new (TX) lists got..." << std::endl;
            done(list);
            std::cout << "Validator.cc: " << "OnRequest(): " << "callback done..." << std::endl;
        }
    });
}


int validator::Listen()
{
    std::cout << "Validator.cc: " << "Listen(): " << "started listening thread..." << std::endl;
    
    while (_listen) {
        std::unique_lock<std::mutex> lck(*_mutex_grpc);
        // std::cout << "Validator.cc: " << "Listen(): " << "waiting for a new request..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        //std:: cout << "new REQ?  " << std::boolalpha << _grpcServer->newRequests() << std::endl;
        _cond_grpc->wait(lck, [this]() { return _grpcServer->newRequests() || !_listen;}); 
        std::cout << "Validator.cc: " << "Listen(): " << "new request(tx) detected..." << std::endl;
        
        std::vector<transaction> tx_list;
        //std::vector<uint32_t> sync_idlist;

        // warning: no more handling of sync req
        // warning : because the static callback function is really a pain to work on with non-static data members
        
        // OnRequest<uint32_t>(std::bind(&comms::ChatServiceImpl::getSyncRequests, _grpcServer, std::ref(sync_idlist)), std::ref(sync_idlist), Event::SYNC, std::bind(&validator::OnSyncReply, this, std::ref(sync_idlist));
        OnRequest<transaction>(std::bind(&comms::ChatServiceImpl::getTransactions, _grpcServer, std::ref(tx_list)), std::ref(tx_list), Event::TX, std::bind(&validator::OnTxReply, this, std::ref(tx_list)));
        
        _mutex_grpc->unlock();
        std::cout << "Validator.cc: " << "Listen(): " << "Onrequest and callback finished..." << std::endl;
        
    }
}


void validator::ConsensusProcess()
{
    std::cout << "Validator.cc: " << "ConsensusConsensus(): " << "running now..." <<std::endl;
    node_->RunProtocol(_consensus);
}


void validator::ProposeProcess()
{
    assert(/*"Only the leader can propose new blocks",*/ node_->isLeader());
    while (_propose) 
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        RunTaskInLoop(Event::PROPOSE, [=]() {
            Block blk;
            if (CreateBlk(blk)) {
                std::cout << "Validator.cc: " << "ProposeConsensus(): " << "blk creation finished with a total " << blk.getTransactions().size() << " txes added" <<std::endl;
                
                Propose(blk);
            }
        });
    }
}


/**
 * @bug: commented for exception raised after line 279, can be wrong capture of lambda function
*/
void validator::DBProcess()
{

     while (_DBstore) 
     {
        std::this_thread::sleep_for(std::chrono::seconds(10));
 
        if (node_->isConsensus()) {
            //std::vector<blockchain::Block> blist = node_->getConsensusBlocks(false);
            /*for (auto&blk : blist) {
                if (find(blk_hashes_.begin(), blk_hashes_.end(), blk) != blk_hashes_.end()) {
                     blk_hashes_.emplace_back(blk);
                }
                else continue;
            }*/
            blist_ = node_->getConsensusBlocks(true);
            std::cout << "Validator.cc: " << "DBProcess(): " << "blk accepted with a total number of " << blist_.size() << "" << std::endl;
            if (!blist_.empty()) {
             RunTaskInLoop(Event::DBSTORE, [=]() {
               //  update the UTXO lists
               for (auto& blk: blist_) {
                   OnAgreeBlk(blk);
                   //db_->addBlock(blk);
                }
                
                // sync with the lists
                std::vector<uint32_t> tlist;
                OnSyncReply(tlist);
                
                // store the block in blockchain
                blist_.clear();
            });
           }
            
          // TODO: start the consensus thread again
          //_consensus_th = std::thread(std::bind(&Validator::ConsensusProcess, this));
       }
       
    }
}


