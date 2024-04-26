#ifndef COSICOIN_WALLET_H
#define COSICOIN_WALLET_H

#include <cstdio>
#include <iostream>
#include <chrono>
#include <algorithm>
// #define NDEBUG
#include <assert.h>

#include "blockchain/transaction.h"
#include "bracha/node.h"
#include "signature/hash.h"
#include "json/jsonparser.h"



namespace cryptowallet {

// T: whether it's Leader or not
class Wallet {
   public:
    Wallet(list<Wallet*> *wallets) wallets_(wallets) {
        pk_generator_ = signature::Signature::getInstance();
        UpdateKeys();
    }

    explicit Wallet(list<Wallet*> *wallets, 
                    uint16_t id,
                    std::string ipaddr,
                    std::string port) 
                    : id_(id), ipaddr_(ipaddr), port_(port), wallets_(wallets) {
        pk_generator_ = signature::Signature::getInstance();
        UpdateKeys();
    };

    ~Wallet() {
        local_utxo_->clear();
    }

    inline uint16_t getId() const { return id_; }



    /*
     * @brief Creates and sends a transaction to the leader validator
     *
     * @param
     * @return 0 the leader did not receive the transaction or the transaction is invalid.
     * @return 1 the leader received and the transaction is valid.
     */
    int SendTx();

    /*
     * @brief Creates a new transaction
     *
     * @param parses the JSON file "tx.json" to obtain the tx information.
     * @return Transaction
     */
    const Transaction& CreateTx(const std::vector<uint64_t>& values,
                                const std::vector<uint16_t>& out_wallet_ids);

    /*
     * @brief Sends a synchronization request to the leader validator
     *
     * @param
     * @return 0 the leader did not receive the transaction or the transaction is invalid.
     * @return 1 the leader received and the transaction is valid.
     */
    int Sync();
    

   private:
    // Sends the transaction to the validator leader through grpc channels.
    void SendTx2Leader();

    // Broadcasts the synchronization request to every validator through grpc channels.
    void BroadcastSync();

    void UpdateKeys();

    //TODO?
    template <typename T>
    void wallet::checkSet(std::vector<T> set1, std::vector<T> set2) {
        std::for_each_n(set1.begin(), set1.size(), [](auto& n) { n += 10; });
    }


   private:
    std::list<Wallet*> *_wallets;


    const uint16_t id_;
    const std::string ipaddr_;
    const std::string port_;

    static uint16_t tx_id_ = 0;


    Signature pk_generator_;
    SigKey pk_;
    SigKey sk_;
    std::vector<blockchain::UTXO> local_utxo_;

    // std::vector<uint16_t> validators_;


    const uint32_t timeout_th_ = 1000;
    const int threshold_ = bracha::n_ / 2;
};
}  // namespace cryptowallet

#endif