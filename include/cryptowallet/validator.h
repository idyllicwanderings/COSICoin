#ifndef COSICOIN_VALIDATOR_H
#define COSICOIN_VALIDATOR_H

#include "blockchain/block.h"
#include "blockchain/transaction.h"
#include "blockchain/utxo.h"
#include "comms/client.h"
#include "comms/server.h"
#include "signature/hash.h"

namespace cryptowallet {

class Validator {
   public:
    Validator() {
        ;
    }

    Validator(uint16_t id,
              std::string ipaddr,
              std::string port) 
              : id_(id), ipaddr_(ipaddr), port_(port) {

    }

    ~Validator() {
        pk_sets_->clear();
        memory_pool_->clear();
        uxto_sets_->clear();
    }

    inline uint16_t getId() const { return id_; }

   private:
    /*
     * @brief update the current public keys according to the newly received Tx.
     *
     * @param a transaction Tx
     * @return
     */
    void UpdateKeys(const blockchain::Transaction& tx);


    void cryptowallet::Validator::CheckInputConsistent(const blockchain::Transaction& tx);

    /*
     * @brief Upon receiving a new Transaction
     *
     * @param a transaction Tx
     * @return 0 a Tx is invalid and discarded
     * @return 1 a Tx is received
     */
    int UponRecvTx(const blockchain::Transaction& tx, uint16_t sender_id);

    /*
     * @brief creates a new block and broadcast the newly proposed block to the other validators.
     *
     * @param a transaction Tx
     * @return 1 creat success
     * @return 0 creat fail
     */
    int CreateBlk();

    /*
     * @brief synchronize with the wallet
     *
     * @param a transaction Tx
     * @return 1 changes added to the wallet's coins
     * @return 0 no changes are added to the wallet
     */
    int SyncWallet(uint16_t wallet_id, const uxto);

    /*
     * @brief polling and listening for any sync/sendtx requests from any wallets.
     *
     * @param
     * @return
     */
    void Listen4Req();

    // intersection of two containers
    template <typename T>
    friend vector<T> vectors_intersection(const vector<T>& v1,
                                          const vector<T>& v2){
        vector<T> v;
        sort(v1.begin(),v1.end());   
        sort(v2.begin(),v2.end());   
        set_intersection(v1.begin(),v1.end(),v2.begin(),v2.end(),back_inserter(v)); 
	    return v;
    }


   private:
    
    const uint16_t id_;
    const std::string ipaddr_;
    const std::string port_;

    comms::ChatServiceImpl server_;
    comms::ChatClientImpl client_;

    Node* node node_;

    SigKey my_pk_;

    // leveldb::DB* blockchain_DB_;

    std::list<Validator*> *validators_;
    //std::list<uint16_t> wallets_;

    /
    std::unordered_map<uint16_t, SigKey> pk_sets_;
    std::unordered_map<uint16_t, std::vector<blockchain::UTXO>> uxto_sets_;
    std::vector<blockchain::Transaction> memory_pool_;
};

}  // namespace cryptowallet

#endif