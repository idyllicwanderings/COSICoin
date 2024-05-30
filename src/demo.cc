/* */
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <assert.h>
#include <mutex>
#include <unistd.h>
#include <functional>

#include "cryptowallet/validator.h"
#include "cryptowallet/wallet.h"
#include "config/settings.h"
#include "bracha/node.h"
#include "bracha/logging.h"


/**
  * a sequential execution of sendTx, createBlk, runConsensus, and addBlockchain.
  * v = 4, w = 3, f = 0
*/
int main() {

   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   /* ++++++++++++++++++++++++++++++Configuration Stage++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
 
   // General configurations of the system.
   config::Settings args("../../settings_demo.json");// configuration file for the demo
   int n = args.getTotalNumberOfValidators();        // total number of validators
   int f = args.getNumberOfFaultyValidators();       // number of faulty validators
   
   std::vector<cryptowallet::Validator> validators;  // fellow validators (the first is the leader)
   std::vector<cryptowallet::Wallet> wallets;        // wallets
   std::vector<uint32_t> wids = args.getWallets();   // wallet ids
   
   int w = args.getWallets().size();                 // number of wallets
   
   std::mutex* io_mtx;                             
   
   // Instantiate the wallets.
   for (const auto& w: args.getWallets()) {
       using namespace cryptowallet;
       wallets.push_back(Wallet(args, w));
   }
   
   std::cout << "GLOBAL: " << w << " wallets instantiated" << std::endl;
   
   // Instantiate the gRPC servers.
   std::vector<comms::ChatServiceImpl*> servers;
   
   comms::ChatServiceImpl* server0 = new comms::ChatServiceImpl;
   servers.push_back(server0);
   std::thread server0_thread(&comms::ChatServiceImpl::Run, servers[0], args.getValidators()[0].port);
   
   comms::ChatServiceImpl* server1 = new comms::ChatServiceImpl;
   servers.push_back(server1);
   std::thread server1_thread(&comms::ChatServiceImpl::Run, servers[1], args.getValidators()[1].port);
   
   comms::ChatServiceImpl* server2 = new comms::ChatServiceImpl;
   servers.push_back(server2);
   std::thread server2_thread(&comms::ChatServiceImpl::Run, servers[2], args.getValidators()[2].port);
   
   comms::ChatServiceImpl* server3 = new comms::ChatServiceImpl;
   servers.push_back(server3);
   std::thread server3_thread(&comms::ChatServiceImpl::Run, servers[3], args.getValidators()[3].port);
  
   
   std::cout << "GLOBAL: " << n << " servers instantiated and started" << std::endl;
   
   // Instantiate the nodes.
   using namespace bracha;
   std::vector<bracha::Node*> nodes;
   
   nodes.push_back(std::move(new LeaderNode("server_node0", 0, args, servers[0],io_mtx)));
   nodes.push_back(std::move(new Node("server_node1", 1, args, servers[1],io_mtx)));
   nodes.push_back(std::move(new Node("server_node2", 2, args, servers[2],io_mtx)));
   nodes.push_back(std::move(new Node("server_node3", 3, args, servers[3],io_mtx)));
   
   assert(nodes[0]->isLeader());               
   
   std::cout << "GLOBAL: " << n << " nodes instantiated" << std::endl;
  
   // Instantiate the validators.
   using namespace cryptowallet;
      
   for (int i = 0; i < n; i++) {
       auto v = args.getValidators()[i];
       validators.emplace_back(std::move(Validator(v.id,args, servers[i], std::move(nodes[i]),wids)));
   }
   
   std::cout << "GLOBAL: " << n << " validators instantiated" << std::endl;
   
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   /* ++++++++++++++++++++++++++++++Genesis Block Stage++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   
   // Firstly, validators distribute the initial coins via transaction where every wallet has 100 coins.
   // Assumption: these transactions are not recorded or represented in any block or the blockchain, but only in UTXOs.
   for (int i = 0; i < n; i++) validators[i].InitializeCoins(100);
   
   for (int i = 0;i < n; i++) validators[i].Run();
   
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++Transaction  Stage++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

   std::cout << "GLOBAL: "<< "initialization worked" << std::endl;

   // A wallet sends one transaction request(details in the json file) to the leader validator,
   // before processing the wallets, the wallets first synchronize with the leader validator.
   wallets[0].SendTx("tx1.json");
   wallets[1].SendTx("tx2.json");
   std::this_thread::sleep_for(std::chrono::seconds(2));

   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++Block Creation Stage++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    
   // The leader waits for a time interval and proposed a new block based on any new valid transactions it has received. Afterwards when a consensus is reached, it updates the UTXO lists and just adds to the database.
   
   std::this_thread::sleep_for(std::chrono::seconds(40)); 
   
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++Verifying Stage+++++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   
   // Checks if the first wallet's UTXO list has been updated.
   std::cout << "GLOBAL: " << "wallet " << 0 << "'s " << wallets[0].GetLocalUTXO() << std::endl;
   wallets[0].Sync();
   std::this_thread::sleep_for(std::chrono::seconds(10)); 
   std::cout << "GLOBAL: " << "wallet " << 0 << "'s " << wallets[0].GetLocalUTXO() << std::endl;
   
   // Checks if the leader validator has updated with the accepted blocks.
   
   
   // @bug: Checks if the second wallet's UTXO list has been updated.
   std::cout << "GLOBAL: " << "wallet " << 1 << "'s " << wallets[1].GetLocalUTXO() << std::endl;
   wallets[1].Sync();
   std::this_thread::sleep_for(std::chrono::seconds(10)); 
   std::cout << "GLOBAL: " << "wallet " << 1 << "'s " << wallets[1].GetLocalUTXO() << std::endl;
   
   // @exception : Prints the database of the blockchain into json files.
   //validators[0].SaveBlockchain("../../blockchain0.json");
   //validators[1].SaveBlockchain("../../blockchain1.json");
   //validators[2].SaveBlockchain("../../blockchain2.json");
   //validators[3].SaveBlockchain("../../blockchain3.json");
   
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++Shutdown Stage+++++++++++++++++++++++++++++++++++++++++++++++*/
   /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
   

   // Join the threads
   for (int i = 0;i < n; i++) {
       validators[i]._blkpropose_th.join();
       validators[i]._consensus_th.join();
       validators[i]._listen_th.join();
       validators[i]._DBstore_th.join();
   }
   
}

