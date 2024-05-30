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

// v = 3, w = 3, f = 0
// sendtx
int main() {
   // General configurations of the system.
   config::Settings args("../../settings_demo.json");
   int n = args.getTotalNumberOfValidators();        // total number of validators
   int f = args.getNumberOfFaultyValidators();       // number of faulty validators
   
   std::vector<cryptowallet::Validator> validators;  // validators
   std::vector<cryptowallet::Wallet> wallets;
   std::vector<uint32_t> wids = args.getWallets();
   
   int w = args.getWallets().size();
   
   std::mutex* io_mtx;
   
   //first initialize the wallets
   for (const auto& w: args.getWallets()) {
       using namespace cryptowallet;
       wallets.push_back(Wallet(args, w.id, w.address, w.port));
       wids.push_back(w.id);
   }
   
   
   // then the gRPC server
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
   
   /*
   for (int i =0;i < n;i++) {
       comms::ChatServiceImpl* server1 = new comms::ChatServiceImpl;
       servers.push_back(server1);
       auto port = args.getValidators()[i].port;
       std::thread server1_thread(&comms::ChatServiceImpl::Run, servers[i], port);
   }*/
   
   std::cout << "servers finished" << std::endl;
   
   // write the node's
   using namespace bracha;
   std::vector<bracha::Node*> nodes;
   nodes.push_back(std::move(new LeaderNode("server_node0", 0, args, servers[0],io_mtx)));
   std::cout << "new a node" << std::endl;
   nodes.push_back(std::move(new Node("server_node1", 1, args, servers[1],io_mtx)));
   nodes.push_back(std::move(new Node("server_node2", 2, args, servers[2],io_mtx)));
   nodes.push_back(std::move(new Node("server_node3", 3, args, servers[3],io_mtx)));
   
   assert(nodes[0]->isLeader());
  
   //instantiate the validators
   using namespace cryptowallet;
   
   // auto v = args.getValidators()[0];
   //Validator leader(v.id,args, servers[0], nodes[0],wids);
   //leader.InitializeCoins();
   
   for (int i =0; i < n;i++) {
       auto v = args.getValidators()[i];
       validators.emplace_back(std::move(Validator(v.id,args, servers[i], std::move(nodes[i]),wids)));
   }
   
   // distribute the firstcoins, everyone has 100
   validators[0].InitializeCoins();

   assert(wallets[0].local_utxo_.getUTXOList().size() == 0);
   wallets[0].Sync();
   assert(wallets[0].local_utxo_.getUTXOList().size() == 1);
   std::cout << "GLOBAL: "<< "initialization worked" << std::endl;
   //std::string s = args.getLeaderAddress();
   //std::cout << "leader addr is   " << s << std::endl;
   
   //start the validator
   validators[0].Run();

   //validators[0]._listen_th.detach();

   //send one Tx_request
   assert(validators[0].memory_pool_.size() == 0);
   wallets[0].SendTx();
   //validators[0]._listen_th.join();
   std::this_thread::sleep_for(std::chrono::seconds(2));
   //assert(validators[0].memory_pool_.size() == 1);
  
   
   std::this_thread::sleep_for(std::chrono::seconds(35)); 
   assert(validators[0].memory_pool_.size() == 0);

   //create one 
   // join the threads
   validators[0]._blkpropose_th.join();
   validators[0]._listen_th.join();
   
   //TODO: join the eventcenters
}
