#include "cryptowallet/wallet.h"

using namespace blockchain;
using namespace hashing;

using wallet = cryptowallet::Wallet;
using jparser = jsonparser;



int wallet::SendTx() {

    // before sending the transaction, synchronize the unspent coins
    Sync();

    // parse user's json input by "tx.json"
    jsonparser::Parser tx_parser_;
    jparser::Transinfo t;
    jparser::from_json(tx_parser_.jf, t);

    // create a new transaction
    Transaction tx = CreateTx(t.value_list_, t.out_walletID_list_);

    // TODO: communinates with the leader validator.
   if (!SendTx2Leader(this->id_, tx)) {
     std::cout << "The transaction's signature or spending conditions is invalid." << std::endl;
   }
   else {
     std::cout << "The transaction successfully sent!" << std::endl;
   }

    return 0;
}




// creates a new transaction.
const Transaction& cryptowallet::Wallet::CreateTx(const std::vector<uint64_t>& values,     
                                    const std::vector<uint16_t>& out_wallet_ids) {

    // assersion fails if values and outputs are incompatible.
    assert(("Values and Output wallets should have the same size", 
            values.size() == out_wallet_ids.size()));
    
    // update the tx identifer
    uint16_t txID = tx_id_++; //TODO;

    std::vector<blockchain::Input> inputs;
    std::vector<blockchain::Output> outputs;

    for (int i = 0; i < values.size(); i++) {
        // check if the value is non-zero
        assert(("Spending values must be non-zero", 
                values[i] >= 0));

        inputs.push_back(new Input(txID,outputIndex));
        outputs.push_back(new Output(values[i], out_wallet_ids[i]));
    }
    // TODO: when the transaction is discarded, but the key should be updated.
    UpdateKeys();

    blockchain::Transaction tx = Transaction(inputs,
                                             outputs,
                                             //senderSig,
                                             this->id_, 
                                             txID,
                                             pk_);
    // TODO: attach the signature with the block.
    unsigned char* signature;

    pk_generator_.Sign(tx.getDigest(), signature);
    tx.setSenderSig(signature);

    return tx;
    
}


//TODO: update message
void cryptowallet::Wallet::BroadcastSync() {
    client_.BroadcastMessage(this->id_, this->local_utxo_);
}

//TODO: update message
void cryptowallet::Wallet::SendTx2Leader(const Transaction& tx) {
    client_.BroadcastMessage(this->id_, tx);
}



 
// TODO: broadcasting the synchronize requests to all the servers would be a pain,
// now we only sync with the leader.
int cryptowallet::Wallet::Sync() {
    
    local_utxo_->clear();

    // broadcast the synchronize requests to all the servers.

    //TODO: right now we only Sync with the leader validator
    // how to write the utxo sets, do we keep it in a temporary list 
    // or we let the grpc channels return the updated utxo set.
    // but you cannot return by reference.

    auto time_start = std::chrono::steady_clock::now();
    //int resp_cnt = BroadcastSync(this->local_utxo_);
    return Sync2Leader((this->local_utxo_);

    // TODO: 
    // wait until floor( (n + 1) / 2) validators have responded?
    //             or timeout
    // while (resp_cnt < threshold_) && 
    //       wait_time < timeout_th_) {
    //     std::chrono::milliseconds wait_time = ((std::chrono::steady_clock::now() - t0).count();
    //     std::this_thread::sleep_for(std::chrono::milliseconds(waiting_max_time_)));
    //     wait();
    // }

    // check whether the coins received from the majority are the same
    //return (resp_cnt >= threshold_);
    
}

// generates a new key pair.
void cryptowallet::Wallet::UpdateKeys() {
    pk_generator_.KeyGen();
    pk_ = pk_generator_.getPublicKey();
    sk_ = pk_generator_.getPrivateKey();
}

