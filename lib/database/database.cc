#include "database/database.h"

using namespace database;

void Database::from_json(const json& j) {
    blocks_.clear();
    digests_.clear();
    for (const auto& el : j.at("blockchain")) {
        std::string digest = el.at("digest");
        digests_.push_back(digest);
        blockchain::Block block;
        block.from_string(el.at("block"));
        blocks_[digest] = block;
    }
}

void Database::to_json(json& j) {
    for (std::string digest : digests_) {
        j["blockchain"].push_back({{"digest", digest}, {"block", blocks_[digest].to_string()}});
    }
}

Database::Database(std::string filename) {
    filename_ = filename;

    std::ifstream i(filename);
    if (!i.is_open()) {
        std::cerr << "Could not open database file: " + filename + " -> starting with empty database" << std::endl;
        blocks_.clear();
        digests_.clear();
    } else {
        json j;
        i >> j;

        from_json(j);

        i.close();
    }
}

void Database::save(std::string filename) {
    json j;
    to_json(j);

    std::ofstream o(filename);
    if (!o.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }

    o << j.dump(4);  // Write the JSON object to the file with indentation of 4 spaces
    o.close();
}

void Database::load(std::string filename) {
    filename_ = filename;

    std::ifstream i(filename);
    if (!i.is_open()) {
        std::cerr << "Could not load database file: " + filename + " -> starting with empty database" << std::endl;
        blocks_.clear();
        digests_.clear();
    } else {
        json j;
        i >> j;

        from_json(j);

        i.close();
    }
}

int Database::addBlock(blockchain::Block block) {
    if (digests_.empty() || digests_.back() == block.getHeader().getPrevBlockDigest()) {
        std::string digest = block.getDigest();
        blocks_[digest] = block;
        digests_.push_back(digest);
        save(filename_);
        return 0;
    }
    return 1;
}

blockchain::Block Database::getBlock(std::string block_digest) {
    if (blocks_.count(block_digest) > 0) {
        return blocks_[block_digest];
    }
    throw std::runtime_error("No block with digest " + block_digest + " in blockchain");
}

std::vector<std::string> Database::getBlockDigests() {
    return digests_;
}
