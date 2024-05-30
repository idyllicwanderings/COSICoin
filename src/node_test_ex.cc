#include <chrono>
#include <mutex>
#include <thread>

#include "blockchain/block.h"
#include "bracha/node.h"
#include "comms/server.h"

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

/* SYSTEM INFO
 * Transaction 3003:
 * Wallet 1:
 *  - ID: 201
 *  - Inputs:
 *    - txID: 3001, index: 0, value: 10
 *    - txID: 3002, index: 1, value: 2
 *  - Outputs:
 *    - value: 6, walletID: 201
 *    - value: 6, walletID: 202
 * Wallet 2:
 *  - ID: 202
 *  - Inputs:
 *    - txID: 3001, index: 2, value: 8
 *  - Outputs:
 *    - value: 1, walletID: 201
 *    - value: 7, walletID: 202
 *
 * Transaction 3004:
 * Wallet 3:
 *  - ID: 203
 *  - Inputs:
 *   - txID: 3001, index: 3, value: 9
 *   - txID: 3002, index: 4, value: 1
 * - Outputs:
 *   - value: 10, walletID: 203
 */

int main() {
    // Create settings
    config::Settings settings = get_local_settings(3, 2);
    // config::Settings settings("../../settings_demo.json");
    const int SLEEP = 100;
    std::mutex io_mutex;

    // --- First consensus run ---
    std::cout << "--- First consensus run ---" << std::endl;

    // Create inputs
    blockchain::Input input11(3001, 0);
    blockchain::Input input12(3002, 1);
    blockchain::Input input21(3001, 2);
    blockchain::Input input31(3001, 3);
    blockchain::Input input32(3002, 4);

    // Create outputs
    blockchain::Output output11(6, 201);
    blockchain::Output output12(6, 202);
    blockchain::Output output21(1, 201);
    blockchain::Output output22(7, 202);
    blockchain::Output output31(10, 203);

    // Create transactions
    blockchain::Transaction transaction1(3003);
    blockchain::Transaction transaction2(3004);
    blockchain::Transaction transaction3(3005);
    transaction1.addInput(input11);
    transaction1.addInput(input12);
    transaction1.addOutput(output11);
    transaction1.addOutput(output12);
    transaction2.addInput(input21);
    transaction2.addOutput(output21);
    transaction2.addOutput(output22);
    transaction3.addInput(input31);
    transaction3.addInput(input32);
    transaction3.addOutput(output31);

    // Create utxolist
    blockchain::UTXOlist utxolist;
    utxolist.add(blockchain::UTXO(3001, 0, blockchain::Output(10, 201)));
    utxolist.add(blockchain::UTXO(3002, 1, blockchain::Output(2, 201)));
    utxolist.add(blockchain::UTXO(3001, 2, blockchain::Output(8, 202)));
    utxolist.add(blockchain::UTXO(3001, 3, blockchain::Output(9, 203)));
    utxolist.add(blockchain::UTXO(3002, 4, blockchain::Output(1, 203)));
    std::vector<uint32_t> receiverIDs = {201, 202, 203};

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
    blockchain::Block block(101, "prevblock");
    block.addTransaction(transaction1);
    block.addTransaction(transaction2);
    block.addTransaction(transaction3);
    block.finalize();
    assert(block.verify(utxolist, receiverIDs));

    // Create servers
    std::cout << "Starting servers" << std::endl;
    comms::ChatServiceImpl serverLeader;
    comms::ChatServiceImpl server1;
    comms::ChatServiceImpl server2;
    std::thread serverLeader_thread(&comms::ChatServiceImpl::Run, &serverLeader, settings.getLeaderInfo().port);
    std::thread server1_thread(&comms::ChatServiceImpl::Run, &server1, settings.getValidatorInfo(1).port);
    std::thread server2_thread(&comms::ChatServiceImpl::Run, &server2, settings.getValidatorInfo(2).port);

    // Create nodes
    std::cout << "Starting nodes" << std::endl;
    bracha::LeaderNode nodeLeader("leader", 0, settings, &serverLeader, &io_mutex);
    bracha::Node node1("node1", 1, settings, &server1, &io_mutex);
    bracha::Node node2("node2", 2, settings, &server2, &io_mutex);
    nodeLeader.setUTXOlists(utxolist, receiverIDs);
    node1.setUTXOlists(utxolist, receiverIDs);
    node2.setUTXOlists(utxolist, receiverIDs);

    // Copy nodes
    std::cout << "Copying nodes" << std::endl;
    bracha::LeaderNode nodeLeader_copy(nodeLeader);
    bracha::Node node1_copy(node1);
    std::cout << "Nodes copied" << std::endl;

    // Move nodes
    std::cout << "Moving nodes" << std::endl;
    bracha::LeaderNode nodeLeader_move(std::move(nodeLeader_copy));
    bracha::Node node1_move(std::move(node1_copy));
    std::cout << "Nodes moved" << std::endl;

    // Check getters
    assert(0 == nodeLeader.getId());
    assert(1 == node1.getId());
    assert(2 == node2.getId());
    assert("leader" == nodeLeader.getName());
    assert("node1" == node1.getName());
    assert("node2" == node2.getName());
    assert(nodeLeader.isLeader());
    assert(!node1.isLeader());
    assert(!node2.isLeader());
    assert(nodeLeader.getConsensusBlocks(false).size() == 0);
    assert(node1.getConsensusBlocks(false).size() == 0);
    assert(node2.getConsensusBlocks(false).size() == 0);
    std::cout << "Getters test succeeded" << std::endl;

    // Running protocol
    std::cout << std::endl
              << "Running protocol" << std::endl;
    std::thread nodeLeader_thread(&bracha::LeaderNode::RunProtocol, &nodeLeader, true);
    std::thread node1_thread(&bracha::Node::RunProtocol, &node1, true);
    std::thread node2_thread(&bracha::Node::RunProtocol, &node2, true);
    while (nodeLeader.isConsensus() || node1.isConsensus() || node2.isConsensus()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    assert(!nodeLeader.isConsensus());
    assert(!node1.isConsensus());
    assert(!node2.isConsensus());
    std::cout << "Leader proposes block: " << block.getHeader().getID().substr(0, 10) << std::endl;
    nodeLeader.Propose(block);
    std::cout << "Waiting for consensus" << std::endl;
    bool leader_con, node1_con, node2_con = false;
    while (!(leader_con && node1_con && node2_con)) {
        if (nodeLeader.isConsensus() != leader_con) {
            std::cout << "Leader consensus: " << nodeLeader.isConsensus() << std::endl;
            leader_con = nodeLeader.isConsensus();
        }
        if (node1.isConsensus() != node1_con) {
            std::cout << "Node1 consensus: " << node1.isConsensus() << std::endl;
            node1_con = node1.isConsensus();
        }
        if (node2.isConsensus() != node2_con) {
            std::cout << "Node2 consensus: " << node2.isConsensus() << std::endl;
            node2_con = node2.isConsensus();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << "All nodes reached consensus" << std::endl;
    std::string voted_hash_leader = nodeLeader.getVotedHash();
    std::string voted_hash_node1 = node1.getVotedHash();
    std::string voted_hash_node2 = node2.getVotedHash();
    std::string block_hash = block.getHeader().getID();
    assert(voted_hash_leader == block_hash);
    assert(voted_hash_node1 == block_hash);
    assert(voted_hash_node2 == block_hash);
    std::cout << "Voted hashes are correct" << std::endl;
    blockchain::Block block_leader = nodeLeader.getReceivedBlock(voted_hash_leader);
    blockchain::Block block_node1 = node1.getReceivedBlock(voted_hash_node1);
    blockchain::Block block_node2 = node2.getReceivedBlock(voted_hash_node2);
    int n = settings.getTotalNumberOfValidators();
    int f = settings.getNumberOfFaultyValidators();
    int min_sigs = 1 + floor((n + f + 1) / 2) + 2 * f + 1;
    assert(block_leader.getValidatorSignatures().size() >= min_sigs);
    assert(block_node1.getValidatorSignatures().size() >= min_sigs);
    assert(block_node2.getValidatorSignatures().size() >= min_sigs);
    std::cout << "All nodes received at least " << min_sigs << " signatures" << std::endl;
    block.removeSignatures();
    block_leader.removeSignatures();
    block_node1.removeSignatures();
    block_node2.removeSignatures();
    assert(block == block_leader);
    assert(block == block_node1);
    assert(block == block_node2);
    std::cout << "Received blocks are correct" << std::endl;
    assert(nodeLeader.getConsensusBlocks(false).size() == 1);
    assert(node1.getConsensusBlocks(false).size() == 1);
    assert(node2.getConsensusBlocks(false).size() == 1);

    // Closing threads
    nodeLeader_thread.join();
    node1_thread.join();
    node2_thread.join();

    // Copy nodes
    std::cout << "Copying nodes" << std::endl;
    bracha::LeaderNode nodeLeader_copy2(nodeLeader);
    bracha::Node node1_copy2(node1);
    std::cout << "Nodes copied" << std::endl;

    // Move nodes
    std::cout << "Moving nodes" << std::endl;
    bracha::LeaderNode nodeLeader_move2(std::move(nodeLeader_copy2));
    bracha::Node node1_move2(std::move(node1_copy2));
    std::cout << "Nodes moved" << std::endl;

    // --- Second consensus run ---
    std::cout << std::endl
              << "--- Second consensus run ---" << std::endl;

    // Create inputs
    blockchain::Input input41(3001, 10);
    blockchain::Input input42(3002, 8);

    // Create outputs
    blockchain::Output output41(7, 201);

    // Create transactions
    blockchain::Transaction transaction4(3006);
    transaction4.addInput(input41);
    transaction4.addInput(input42);
    transaction4.addOutput(output41);

    // Create utxolist
    utxolist.clear();
    utxolist.add(blockchain::UTXO(3001, 10, blockchain::Output(4, 201)));
    utxolist.add(blockchain::UTXO(3002, 8, blockchain::Output(3, 201)));

    // add signature
    signature = signature::Signature::getInstance();
    signature.KeyGen();
    std::vector<std::string> signat4 = signature::Sign(transaction4.getDigest(), signature.getPrivateKey());
    transaction4.setSenderSig(signat4);
    transaction4.setPublicKey(signature.getPublicKey());

    // Create block
    blockchain::Block block2(102, block.getHeader().getID());
    block2.addTransaction(transaction4);
    block2.finalize();
    assert(block2.verify(utxolist, receiverIDs));

    std::cout << "Updating the nodes" << std::endl;
    nodeLeader.setUTXOlists(utxolist, receiverIDs);
    node1.setUTXOlists(utxolist, receiverIDs);
    node2.setUTXOlists(utxolist, receiverIDs);

    // Running protocol
    std::cout << "Running protocol" << std::endl;
    nodeLeader_thread = std::thread(&bracha::LeaderNode::RunProtocol, &nodeLeader, true);
    node1_thread = std::thread(&bracha::Node::RunProtocol, &node1, true);
    node2_thread = std::thread(&bracha::Node::RunProtocol, &node2, true);
    while (nodeLeader.isConsensus() || node1.isConsensus() || node2.isConsensus()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << "Nodes are not in consensus" << std::endl;
    assert(!nodeLeader.isConsensus());
    assert(!node1.isConsensus());
    assert(!node2.isConsensus());
    std::cout << "Leader proposes block: " << block2.getHeader().getID().substr(0, 10) << std::endl;
    nodeLeader.Propose(block2);
    std::cout << "Waiting for consensus" << std::endl;
    leader_con = false;
    node1_con = false;
    node2_con = false;
    while (!(leader_con && node1_con && node2_con)) {
        if (nodeLeader.isConsensus() != leader_con) {
            std::cout << "Leader consensus: " << nodeLeader.isConsensus() << std::endl;
            leader_con = nodeLeader.isConsensus();
        }
        if (node1.isConsensus() != node1_con) {
            std::cout << "Node1 consensus: " << node1.isConsensus() << std::endl;
            node1_con = node1.isConsensus();
        }
        if (node2.isConsensus() != node2_con) {
            std::cout << "Node2 consensus: " << node2.isConsensus() << std::endl;
            node2_con = node2.isConsensus();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << "All nodes reached consensus" << std::endl;
    voted_hash_leader = nodeLeader.getVotedHash();
    voted_hash_node1 = node1.getVotedHash();
    voted_hash_node2 = node2.getVotedHash();
    block_hash = block2.getHeader().getID();
    assert(voted_hash_leader == block_hash);
    assert(voted_hash_node1 == block_hash);
    assert(voted_hash_node2 == block_hash);
    std::cout << "Voted hashes are correct" << std::endl;
    block_leader = nodeLeader.getReceivedBlock(voted_hash_leader);
    block_node1 = node1.getReceivedBlock(voted_hash_node1);
    block_node2 = node2.getReceivedBlock(voted_hash_node2);
    min_sigs = 1 + floor((n + f + 1) / 2) + 2 * f + 1;
    assert(block_leader.getValidatorSignatures().size() >= min_sigs);
    assert(block_node1.getValidatorSignatures().size() >= min_sigs);
    assert(block_node2.getValidatorSignatures().size() >= min_sigs);
    std::cout << "All nodes received at least " << min_sigs << " signatures" << std::endl;
    block2.removeSignatures();
    block_leader.removeSignatures();
    block_node1.removeSignatures();
    block_node2.removeSignatures();
    assert(block2 == block_leader);
    assert(block2 == block_node1);
    assert(block2 == block_node2);
    std::cout << "Received blocks are correct" << std::endl;
    assert(nodeLeader.getConsensusBlocks().size() == 2);
    assert(node1.getConsensusBlocks().size() == 2);
    assert(node2.getConsensusBlocks().size() == 2);

    // Closing threads
    nodeLeader_thread.join();
    node1_thread.join();
    node2_thread.join();

    // // --- Faulty consensus run ---                      uncomment this part to simulate a faulty node but then the tests will not succeed
    // std::cout << std::endl
    //           << "--- Faulty consensus run ---" << std::endl;

    // // Create inputs
    // blockchain::Input input51(3001, 10);
    // blockchain::Input input52(3002, 8);

    // // Create outputs
    // blockchain::Output output51(7, 201);

    // // Create transactions
    // blockchain::Transaction transaction5(3006);
    // transaction5.addInput(input51);
    // transaction5.addInput(input52);
    // transaction5.addOutput(output51);

    // // Create utxolist
    // utxolist.clear();
    // utxolist.add(blockchain::UTXO(3001, 10, blockchain::Output(4, 201)));
    // utxolist.add(blockchain::UTXO(3002, 8, blockchain::Output(3, 201)));

    // // add signature
    // signature = signature::Signature::getInstance();
    // signature.KeyGen();
    // std::vector<std::string> signat5 = signature::Sign(transaction5.getDigest(), signature.getPrivateKey());
    // transaction5.setSenderSig(signat5);
    // transaction5.setPublicKey(signature.getPublicKey());

    // // Create block
    // blockchain::Block block3(103, block2.getHeader().getID());
    // block3.addTransaction(transaction4);
    // block3.finalize();
    // assert(block3.verify(utxolist, receiverIDs));

    // std::cout << "Updating the nodes" << std::endl;
    // bracha::FaultNode fnode2("fnode2", 2, settings, &server2, &io_mutex);
    // nodeLeader.setUTXOlists(utxolist, receiverIDs);
    // node1.setUTXOlists(utxolist, receiverIDs);
    // fnode2.setUTXOlists(utxolist, receiverIDs);

    // // Running protocol
    // std::cout << "Running protocol" << std::endl;
    // nodeLeader_thread = std::thread(&bracha::LeaderNode::RunProtocol, &nodeLeader, true);
    // node1_thread = std::thread(&bracha::Node::RunProtocol, &node1, true);
    // node2_thread = std::thread(&bracha::FaultNode::RunProtocol, &fnode2, true, bracha::ByzantineBehavior::CRASH);
    // while (nodeLeader.isConsensus() || node1.isConsensus() || node2.isConsensus()) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    // }
    // std::cout << "Nodes are not in consensus" << std::endl;
    // std::cout << "Leader proposes block: " << block3.getHeader().getID().substr(0, 10) << std::endl;
    // nodeLeader.Propose(block3);
    // std::cout << "Waiting for consensus" << std::endl;
    // leader_con = false;
    // node1_con = false;
    // node2_con = false;
    // int timeout = 0;
    // while (!(leader_con && node1_con && node2_con)) {
    //     if (nodeLeader.isConsensus() != leader_con) {
    //         std::cout << "Leader consensus: " << nodeLeader.isConsensus() << std::endl;
    //         leader_con = nodeLeader.isConsensus();
    //     }
    //     if (node1.isConsensus() != node1_con) {
    //         std::cout << "Node1 consensus: " << node1.isConsensus() << std::endl;
    //         node1_con = node1.isConsensus();
    //     }
    //     if (fnode2.isConsensus() != node2_con) {
    //         std::cout << "Node2 consensus: " << fnode2.isConsensus() << std::endl;
    //         node2_con = fnode2.isConsensus();
    //     }
    //     std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    //     timeout++;
    //     if (timeout > 100) {
    //         break;
    //     }
    // }
    // if (timeout >= 100) {
    //     std::cout << "Nodes did not reach consensus, timeout reached" << std::endl;
    // } else {
    //     std::cout << "All nodes reached consensus" << std::endl;
    // }

    std::cout << std::endl
              << "All test succeeded" << std::endl
              << std::endl;

    return 0;
}
