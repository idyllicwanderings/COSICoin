#include <chrono>
#include <thread>

#include "comms/client.h"
#include "comms/server.h"
#include "config/settings.h"

config::Settings get_local_settings(int num_servers, int num_wallets) {
    std::vector<config::AddressInfo> vals;
    for (int i = 0; i < num_servers; i++) {
        config::AddressInfo val;
        val.id = i;
        val.address = "127.0.0.1";
        val.port = 50000 + i;
        val.faulty = false;
        vals.push_back(val);
    }
    std::vector<uint32_t> wals;
    for (int i = 0; i < num_wallets; i++) {
        wals.push_back(100 + i);
    }
    config::Settings settings(vals, 0, wals, -1, -1);
    return settings;
}

int main() {
    // Create settings
    config::Settings settings = get_local_settings(2, 2);
    const int SLEEP = 100;

    // Create inputs
    blockchain::Input input1(2001, 10);
    blockchain::Input input2(2002, 2);
    blockchain::Input input3(2003, 7);

    // Create outputs
    blockchain::Output output1(6, 1001);
    blockchain::Output output2(20, 1002);
    blockchain::Output output3(10, 1002);

    // Create transactions
    blockchain::Transaction transaction1(3001);
    blockchain::Transaction transaction2(3002);
    transaction1.addInput(input1);
    transaction1.addInput(input2);
    transaction2.addInput(input3);
    transaction1.addOutput(output1);
    transaction1.addOutput(output2);
    transaction2.addOutput(output3);

    // add signature
    signature::Signature signature = signature::Signature::getInstance();
    signature.KeyGen();
    std::vector<std::string> signat1 = signature::Sign(transaction1.getDigest(), signature.getPrivateKey());
    std::vector<std::string> signat2 = signature::Sign(transaction2.getDigest(), signature.getPrivateKey());
    transaction1.setSenderSig(signat1);
    transaction2.setSenderSig(signat2);
    transaction1.setPublicKey(signature.getPublicKey());
    transaction2.setPublicKey(signature.getPublicKey());

    // Create block
    blockchain::Block block(2, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);
    block.finalize();

    // Sign block
    signature::Signature signature1 = signature::Signature::getInstance();
    signature::Signature signature2 = signature::Signature::getInstance();
    signature1.KeyGen();
    signature2.KeyGen();
    block.sign(signature1.getPrivateKey(), 26, signature2.getPublicKey());

    // Create UTXOlist
    blockchain::UTXO utxo1(101, 11, output1);
    blockchain::UTXO utxo2(102, 12, output2);
    blockchain::UTXO utxo3(103, 13, output3);
    std::vector<blockchain::UTXO> utxolist = {utxo1, utxo2, utxo3};

    // Create messages
    blockchain::Message messageBlock(blockchain::MsgType::ECHO, block, 1002, 1);

    // Start servers
    std::cout << "Starting servers" << std::endl;
    comms::ChatServiceImpl server1;
    comms::ChatServiceImpl server2;
    std::thread server1_thread(&comms::ChatServiceImpl::Run, &server1, settings.getLeaderInfo().port);
    std::thread server2_thread(&comms::ChatServiceImpl::Run, &server2, settings.getValidatorInfo(1).port);
    std::vector<blockchain::Message> msg_list;

    // Start client
    std::cout << "Starting client" << std::endl;
    comms::ValidatorClientImpl client(settings);

    // Make wallets
    std::cout << "Making wallets" << std::endl;
    comms::WalletClientImpl client_wallet1(settings);
    comms::WalletClientImpl client_wallet2(settings);
    comms::WalletClientImpl client_wallet3(settings);

    // Wait to give servers time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));

    // --- Block broadcast test ---
    std::cout << std::endl
              << "Block broadcast test" << std::endl;
    assert(!server1.newConsensusMessages());
    assert(!server2.newConsensusMessages());
    client.Broadcast(messageBlock);

    std::cout << "Waiting for server1 to receive" << std::flush;
    while (!server1.getMessages(msg_list)) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << std::endl
              << "Received message" << std::endl;
    assert(msg_list.size() == 1);
    blockchain::Block rec_block = msg_list[0].getBlock();
    assert(block == rec_block);
    assert(!server1.newConsensusMessages());

    std::cout << "Waiting for server2 to receive" << std::flush;
    assert(server2.newConsensusMessages());
    msg_list.clear();
    while (!server2.getMessages(msg_list)) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << std::endl
              << "Received message" << std::endl;
    assert(msg_list.size() == 1);
    rec_block = msg_list[0].getBlock();
    assert(block == rec_block);
    assert(!server2.newConsensusMessages());
    std::cout << "Block broadcast test successful" << std::endl;

    // --- Transaction to leader test ---
    std::cout << std::endl
              << "Transaction to leader test" << std::endl;
    assert(!server1.newRequests());
    assert(!server2.newRequests());
    client_wallet1.SendToLeader(transaction1);

    std::cout << "Waiting for leader to receive" << std::flush;
    std::vector<blockchain::Transaction> tx_list;
    while (!server1.getTransactions(tx_list)) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << std::endl
              << "Received transaction" << std::endl;
    std::cout << "txsize: " << tx_list.size() << std::endl;
    assert(tx_list.size() == 1);
    blockchain::Transaction rec_tx = tx_list[0];
    assert(transaction1 == rec_tx);
    assert(!server2.getTransactions(tx_list));
    assert(!server1.newRequests());
    assert(!server2.newRequests());
    std::cout << "Transaction to leader test successful" << std::endl;

    // --- Wallet sync test ---
    std::cout << std::endl
              << "Wallet sync request test" << std::endl;
    blockchain::UTXOlist utxolist_wallet1;
    blockchain::UTXOlist utxolist_wallet2;
    blockchain::UTXOlist utxolist_wallet3;
    assert(!server1.newRequests());
    assert(!server2.newRequests());
    std::unordered_map<uint32_t, blockchain::UTXOlist> utxolists;
    utxolists[1] = utxolist;
    utxolists[2] = utxolist;
    server1.setUTXOlists(utxolists);
    std::cout << "Sending sync requests" << std::endl;
    std::thread wallet1_thread(&comms::WalletClientImpl::SendSyncRequest, &client_wallet1, 1, std::ref(utxolist_wallet1));
    std::thread wallet2_thread(&comms::WalletClientImpl::SendSyncRequest, &client_wallet2, 2, std::ref(utxolist_wallet2));
    std::thread wallet3_thread(&comms::WalletClientImpl::SendSyncRequest, &client_wallet3, 3, std::ref(utxolist_wallet3));
    std::cout << "Waiting for wallet 1 to receive sync reply" << std::endl;
    wallet1_thread.join();
    assert(utxolist == utxolist_wallet1);
    std::cout << "Waiting for wallet 2 to receive sync reply" << std::endl;
    wallet2_thread.join();
    assert(utxolist == utxolist_wallet2);
    std::cout << "Waiting for wallet 3 to receive sync reply" << std::endl;
    blockchain::UTXOlist empty_utxolist;
    wallet3_thread.join();
    assert(empty_utxolist == utxolist_wallet3);
    std::cout << "Wallet sync request test successful" << std::endl;

    // --- Copy test ---
    std::cout << std::endl
              << "Copy client test" << std::endl;
    comms::ValidatorClientImpl client_copy(client);
    client_copy.Broadcast(messageBlock);

    std::cout << "Waiting for server1 to receive" << std::flush;
    msg_list.clear();
    while (!server1.getMessages(msg_list)) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << std::endl
              << "Received message" << std::endl;
    assert(msg_list.size() == 1);
    rec_block = msg_list[0].getBlock();
    assert(block == rec_block);
    assert(!server1.newConsensusMessages());

    std::cout << "Waiting for server2 to receive" << std::flush;
    assert(server2.newConsensusMessages());
    msg_list.clear();
    while (!server2.getMessages(msg_list)) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << std::endl
              << "Received message" << std::endl;
    assert(msg_list.size() == 1);
    rec_block = msg_list[0].getBlock();
    assert(block == rec_block);
    assert(!server2.newConsensusMessages());
    std::cout << "Client copy test successful" << std::endl;

    // --- Move test ---
    std::cout << std::endl
              << "Copy client test" << std::endl;
    comms::ValidatorClientImpl client_move(std::move(client));
    client_move.Broadcast(messageBlock);

    std::cout << "Waiting for server1 to receive" << std::flush;
    msg_list.clear();
    while (!server1.getMessages(msg_list)) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << std::endl
              << "Received message" << std::endl;
    assert(msg_list.size() == 1);
    rec_block = msg_list[0].getBlock();
    assert(block == rec_block);
    assert(!server1.newConsensusMessages());

    std::cout << "Waiting for server2 to receive" << std::flush;
    assert(server2.newConsensusMessages());
    msg_list.clear();
    while (!server2.getMessages(msg_list)) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << std::endl
              << "Received message" << std::endl;
    assert(msg_list.size() == 1);
    rec_block = msg_list[0].getBlock();
    assert(block == rec_block);
    assert(!server2.newConsensusMessages());
    std::cout << "Client copy test successful" << std::endl;

    std::cout << std::endl
              << "All test succeeded" << std::endl
              << std::endl;
}
