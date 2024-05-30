#include "cryptowallet/wallet.h"

using namespace blockchain;

using wallet = cryptowallet::Wallet;



int wallet::SendTx(std::string filename) {

    std::cout << "SendTX: " << "creating a new tx" << std::endl;
    // before sending the transaction, synchronize the unspent coins
    Sync();
    std::cout << "SendTX: " << "sync finished" << std::endl;

    // create a new transaction
    Transaction tx = CreateTx(filename);
    std::cout << "SendTX: " << "create_tx" << "finished" << std::endl;

    // TODO: communinates with the leader validator.
    if (!SendTx2Leader(tx)) {
      std::cout << "Wallet.cc: SendTX: " << "gRPC communication failed" << std::endl;
    }
    else {
      std::cout <<  "Wallet.cc: SendTX: " << "the new transaction successfully sent!" << std::endl;
    }
    return 1;
}



Transaction cryptowallet::Wallet::CreateTx(std::string filename) {
    
    // Paring the tx info from json parser
    using namespace jsonparser;
    jsonparser::Parser tx_parser(filename);
    jsonparser::TransInfo t;
    //std::cout << "CreateTX: " << "json parser declared finished" << std::endl;
    jsonparser::from_json(tx_parser.jf, t);
    //std::cout << "CreateTX: " << "from_json" << "finished" << std::endl;
      
    // update the tx identifer
    uint32_t txID = tx_id_++; //TODO;
    std::vector<blockchain::Input> inputs;
    std::vector<blockchain::Output> outputs;
    std::vector<uint64_t> values = t.out_value_list_;
    std::vector<uint32_t> outIDs = t.out_walletID_list_;
    std::vector<uint32_t> inTXs = t.in_txID_list_;
    std::vector<uint64_t> inIDXs = t.in_outIDX_list_;
    
    for (int i = 0; i < inTXs.size(); i++ ) {
       inputs.push_back(Input(inTXs[i],inIDXs[i]));
    }
    
    for (int i = 0; i < values.size(); i++) {
        // check if the value is non-zero
        assert(("Spending values must be non-zero", 
                values[i] >= 0));
        outputs.push_back(Output(values[i], outIDs[i]));
    }
    
    //when the transaction is discarded, but the key should be updated.
    UpdateKeys();

    blockchain::Transaction tx = Transaction(inputs,
                                             outputs,
                                             //senderSig,
                                             this->id_, 
                                             txID,
                                             pk_);

    // attach the signature with the block.
    using Sig = std::vector<std::string>;
    Sig signature = signature::Sign(tx.getDigest(), pk_generator_.getPrivateKey());
    tx.setSenderSig(signature);

    return tx;
    
}

 
// broadcasting the synchronize requests to all the servers would be a pain,
// now we only sync with the leader 
int cryptowallet::Wallet::Sync() {
  std::cout << "Sync: " << "started" << std::endl;
  local_utxo_.clear();
  //auto time_start = std::chrono::steady_clock::now();
  return _grpcClient.SendSyncRequest(this->id_, this->local_utxo_);
}

// generates a new key pair.
void cryptowallet::Wallet::UpdateKeys() {
    pk_generator_.KeyGen();
    pk_ = pk_generator_.getPublicKey();
    sk_ = pk_generator_.getPrivateKey();
}
