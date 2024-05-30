#ifndef COSICOIN_DATABASE_H
#define COSICOIN_DATABASE_H

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "blockchain/block.h"
#include "json/json.hpp"

using json = nlohmann::json;

namespace database {

class Database {
   public:
    /*
     * Creates new database object using the given file for storage (as json)
     */
    Database(std::string filename);

    /*
     * Add a block to the blockchain in database
     * Returns 0 if block succesfully added (prev block digest is correct)
     * Returns 1 if prev block digest does not match
     */
    int addBlock(blockchain::Block block);

    /*
     * Returns the block with the given digest
     */
    blockchain::Block getBlock(std::string block_digest);

    /*
     * Returns the digests of the blocks in the blockchain
     */
    std::vector<std::string> getBlockDigests();

    /*
     * Saves the database to file
     */
    void save(std::string filename);

    /*
     * Loads the database from file
     */
    void load(std::string filename);

   private:
    std::map<std::string, blockchain::Block> blocks_;  // digest - json_block
    std::vector<std::string> digests_;
    std::string filename_;

    void from_json(const json& j);

    void to_json(json& j);
};


// The global database. 
//extern Database db("../../database.json");

}  // namespace database

#endif
