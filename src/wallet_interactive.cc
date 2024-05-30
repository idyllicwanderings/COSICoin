#include <iostream>
#include <string>
#include <vector>

#include "config/settings.h"
#include "cryptowallet/validator.h"
#include "cryptowallet/wallet.h"
#include "signature/hash.h"

int get_total_coins(blockchain::UTXOlist utxolist) {
    int total_coins = 0;
    for (const auto& utxo : utxolist) {
        total_coins += utxo.getOutput().getValue();
    }
    return total_coins;
}   

blockchain::Transaction create_tx(int amount, int receiver_id, blockchain::UTXOlist utxolist, uint32_t wallet_id) {
    blockchain::Transaction tx;
    for (const auto& utxo : utxolist) {
        if (utxo.getOutput().getValue() > amount) {
            uint32_t txID = utxo.getTransactionId() - static_cast<uint32_t>(amount);
            tx.settxID(txID);
            tx.setSenderID(static_cast<uint16_t>(wallet_id));
            tx.addInput(blockchain::Input(utxo.getTransactionId(), utxo.getOutputIndex()));
            tx.addOutput(blockchain::Output(amount, receiver_id));
            tx.addOutput(blockchain::Output(utxo.getOutput().getValue() - amount, wallet_id));
            break;
        }
        else if (utxo.getOutput().getValue() == amount) {
            uint32_t txID = utxo.getTransactionId() - static_cast<uint32_t>(amount);
            tx.settxID(txID);
            tx.setSenderID(static_cast<uint16_t>(wallet_id));
            tx.addInput(blockchain::Input(utxo.getTransactionId(), utxo.getOutputIndex()));
            tx.addOutput(blockchain::Output(amount, receiver_id));
            break;
        }      
    }
    int sum = 0;
    for (const auto& utxo : utxolist) {
        sum += utxo.getOutput().getValue();
        if (sum > amount) {
            tx.addInput(blockchain::Input(utxo.getTransactionId(), utxo.getOutputIndex()));
            tx.addOutput(blockchain::Output(sum - amount, receiver_id));
            tx.addOutput(blockchain::Output(utxo.getOutput().getValue() - sum + amount, wallet_id));
            break;
        }
        else if (sum == amount) {
            tx.addInput(blockchain::Input(utxo.getTransactionId(), utxo.getOutputIndex()));
            tx.addOutput(blockchain::Output(utxo.getOutput().getValue(), receiver_id));
            break;
        }
        else {
        tx.addInput(blockchain::Input(utxo.getTransactionId(), utxo.getOutputIndex()));
        tx.addOutput(blockchain::Output(utxo.getOutput().getValue(), receiver_id));
        }
    }
    
    tx.settxID(static_cast<uint32_t>(sum));
    tx.setSenderID(static_cast<uint16_t>(wallet_id));   
    return tx;
}

int main() {
    std::cout << "This is a interactive wallet demo" << std::endl;

    // Reading the settings file
    std::cout << "Path to settings file: ";
    std::string settings_path;
    std::cin >> settings_path;
    config::Settings settings(settings_path);

    // Creating the wallet
    uint32_t wallet_id = settings.getMyWalletID();
    std::cout << "Wallet ID: " << wallet_id << std::endl;
    cryptowallet::Wallet wallet(settings, wallet_id);
    signature::Signature next_keypair = signature::Signature::getInstance();
    next_keypair.KeyGen();
    signature::SigKey sk = next_keypair.getPrivateKey();

    // Main loop
    std::string input;
    while (input != "q") {
        // Syncing with the leader
        std::cout << "Syncing with leader" << std::endl;
        int r = wallet.Sync();
        if (r == 0) {
            std::cout << "Sync failed" << std::endl;
        } else {
            std::cout << "Sync successful" << std::endl;
        }
        int coins = get_total_coins(wallet.GetLocalUTXO());
        std::cout << "Your balance is: " << coins << std::endl;
        // Creating new transaction
        std::cout << "Do you want to create a new transaction? (y/N) ";
        std::cin >> input;
        if (input == "y") {
            std::cout << "Creating a new transaction" << std::endl;
            std::cout << "Enter amount: ";
            int amount;
            std::cin >> amount;
            std::cout << "Enter receiver ID: ";
            int receiver_id;
            std::cin >> receiver_id;
            blockchain::Transaction tx = create_tx(amount, receiver_id, wallet.GetLocalUTXO(), wallet_id);
            next_keypair.KeyGen();
            tx.setPublicKey(next_keypair.getPublicKey());
            std::string digest = tx.getDigest();
            std::vector<std::string> signature = signature::Sign(digest, sk);
            tx.setSenderSig(signature);
            sk = next_keypair.getPrivateKey();
            std::cout << "Sending transaction" << std::endl;
            int r = wallet.SendTx2Leader(tx);
            if (r == 0) {
                std::cout << "Transaction send failed" << std::endl;
            } else {
                std::cout << "Transaction send successful" << std::endl;
            }
        }
    }
}