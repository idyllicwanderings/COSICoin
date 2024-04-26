#include"cryptowallet/validator.h"


using namespace cryptowallet;
using namespace hashing;
using namespace blockchain;


void cryptowallet::Validator::UpdateKeys(const blockchain::Transaction& tx) {
    pk_sets_[sender_id] = tx.getPublicKey();
}

void cryptowallet::Validator::CheckInputConsistent(const blockchain::Transaction& tx) {

    std::vector<blockchain::Input> inputs = tx.getInputs();

    for (const auto& input: inputs ) {
        bool flag = false;
        for (const auto& output : uxto_sets_[wallet_id]) {
            if (input.getTxID() == out.getTransactionId() 
                && input.getOutputIndex() == out.getOutputIndex()) {
                flag = true;
                break;
            }
        }
        if (!flag) return 0;
    }

}


int cryptowallet::Validator::UponRecvTx(const blockchain::Transaction& tx, 
                                        uint16_t sender_id) {

    uint16_t wallet_id = tx.getSenderId();

    // checks the signature of the Sign(tx) using PK_{i,n}
    int valid = hashing::Verify(tx.getDigest(), 
                                tx.getSenderSig(),
                                pk_sets_[sender_id_]);

    // updates the PK set < id_i,PK_{n+1}>
    // Notes: the PK gets updated despite the invalidity of signature.
    UpdateKeys(tx);

    if (!valid) return 0;

    /* checks whether 
     * 1) the output(h,i) is in the local UTXO set
     * 2) spending conditions(drop when not sum of inputs == sum of outputs).
     * 3) signature is valid
     */ 
    if (!valid || CheckInputConsistent(tx) || !tx.checkSpendingConditions()) {
        return 0;
    }

    // adds to the tx memory pool
    memory_pool_.push_back(tx);

    // TODO: Notify the thread
    return 1;

}


int cryptowallet::Validator::CreateBlk() {

    // check the node is a leader 
    static_assert(node_->isLeader());

    Block bx;
    std::vector<Transaction> confirmed_tx;

    while (!memory_pool_.empty()) {
        Transaction cur_tx = memory_pool_.pop_back();
        std::vector<UTXO> cur_utxo = uxto_sets_[cur_tx.getSenderId()];
        
        // check whether the tx is consistent with local UTXO set

        // the tx is consistent with the UTXOs
        std::for_each_n(confirmed_tx.begin(), confirmed_tx.size(), 
                        [](auto& t) { vectors_intersection( t.getOutputs(),
                                                            cur_tx.getOuputs())
                                                           .size()
                                                         == 0; });
        
        // check whether the tx is consistent with the bx( double spending)
        if (!bx.verify(cur_tx)) {
            continue;
        }

        // remove the current transaction from the memory pool.
        memory_pool_.remove(cur_tx);
        confirmed_tx.push_back(cur_tx);

        // update the local UTXO sets
        std::vector<blockchain::Input> inputs = cur_tx.getInputs();

        for (int i = 0; i < inputs.size(); i++) {
            Input cur_in = inputs[i];
            Output cur_out = inputs.getInputAt(i);
            UTXO cur_utxo = UTXO(cur_tx.getId(),
                                 i,
                                 cur_out);
            utxo_sets_[cur_tx.getSenderId()].push_back(cur_utxo);
        }

    }

    // propose bx to the other validators
    node_->propose(bx);

}


int cyrptowallet::Validator:: SyncWallet(uint16_t wallet_id, const std::vector<UTXO>& utxo) {
   utxo = utxo_sets[wallet_id].clone();
   return 1;
}


void cyrptowallet::Validator:: Listen4Req() {
    do {
        // listen for channel requests
        // TODO: listen4sync returns 0 if no requests, otherwise it returns a list of wallet ids
        if (Listen4Sync(vector<std::uint16_t>)) {
            // new a thread for coping with sync
            UponRecvTx();
        }

        // listens for Tx
        if (listen4Tx()) {
            // new a thread for coping with the tx
            
        }
    } while (!down);
}

// One thread for executing the protocol(one server thread also).
// One thread for coping with the sync requests (lifecycle: dies when the transaction is processed)
// One thread for coping with the tx requests, same lifecycle.
void cyrptowallet::Validator:: Run() {
    node_->RunProtocol();
    listen4Req();
}