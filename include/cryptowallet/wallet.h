#ifndef COSICOIN_WALLET_H
#define COSICOIN_WALLET_H

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
// #define NDEBUG
#include <assert.h>

#include <condition_variable>
#include <mutex>
#include <vector>

#include "blockchain/block.h"
#include "blockchain/transaction.h"
#include "bracha/node.h"
#include "comms/client.h"
#include "comms/server.h"
#include "config/settings.h"
#include "json/jsonparser.h"
#include "signature/hash.h"

namespace cryptowallet {

class Wallet {
   public:
    explicit Wallet(/*std::list<Wallet*> *wallets,*/
                    config::Settings settings,
                    uint32_t id)
        : id_(id), _grpcClient(settings), pk_generator_(signature::Signature::getInstance()), n_(settings.getTotalNumberOfValidators())
    /*, _grpcClient(grpcClient)*/ {
        UpdateKeys();
    };

    ~Wallet() {
        local_utxo_.clear();
    }

    /**
     * @brief set all other wallets.
    */
    void setWalletPeers(std::list<Wallet*> wallets) {
        _wallets = wallets;
    }

    inline uint32_t getId() const { return id_; }

    /*
     * @brief Sends a transaction via parsing "filename" to the leader validator
     *
     * @param
     * @return 0 the leader did not receive the transaction or the transaction is invalid.
     * @return 1 the leader received and the transaction is valid.
     */
    int SendTx(std::string filename);

    /**
     * @brief Sends a synchronization request to the leader validator
     *
     */
    int Sync();

    /**
     * @brief Get the local UTXO list
     */
    blockchain::UTXOlist GetLocalUTXO() { return local_utxo_; }

    /**
     * @brief Send a new transaction to leader
     */
    int SendTx2Leader(const blockchain::Transaction& tx) {
        return _grpcClient.SendToLeader(tx);
    }

   private:
    /**
     * @brief Creates a new transaction
     *
     * @param parses the JSON file "filename" (src/tx directory) to obtain the tx information.
     * @return Transaction
    */
    blockchain::Transaction CreateTx(std::string filename);

    void UpdateKeys();

    /**
     * @brief Deprecated due to failure of cross-compilation for for_each_n
     */
    // template <typename T>
    // void checkSet(std::vector<T> set1, std::vector<T> set2) {
    //     std::for_each_n(set1.begin(), set1.size(), [](auto& n) { n += 10; });
    // }

    /**
     * @attention TODO: uncomment for real use, public for testing
     */
    // private:
   public:
    // gRPC instance
    comms::WalletClientImpl _grpcClient;

    std::list<Wallet*> _wallets;

    // configurations
    const uint32_t id_;
    const int n_;

    // starts from 100, tx id < 100 is used for the first coins
    static inline uint32_t tx_id_ = 100;

    // UTXO and key sets
    signature::Signature pk_generator_;
    signature::SigKey pk_;
    signature::SigKey sk_;
    blockchain::UTXOlist local_utxo_;

    // std::vector<uint32_t> validators_;

    const uint32_t timeout_th_ = 1000;
    const int threshold_ = n_ / 2;
};
}  // namespace cryptowallet

#endif
