#include "blockchain/header.h"

using namespace blockchain;

/*
Header::Header(const chat::Header& proto_header) {
    prevBlockDigest_ = proto_header.prevblockdigest();
    merkleRoot_ = proto_header.merkleroot();
}
*/

/*
chat::Header Header::toProtoHeader() const {
    chat::Header proto_header;

    proto_header.set_prevblockdigest(prevBlockDigest_);
    proto_header.set_merkleroot(merkleRoot_);

    return proto_header;
}
*/

void Header::calculateMerkleRoot(std::vector<Transaction> transactions) {
    // If there are no transactions, return empty string
    if (transactions.empty()) {
        merkleRoot_ = "";
        return;
    }

    // Calculate hash of each transaction
    std::vector<std::string> hashes;
    hashes.reserve(transactions.size());
    for (const auto& transaction : transactions) {
        hashes.push_back(transaction.getDigest());
    }

    // Repeat until only one hash left
    while (hashes.size() > 1) {
        // Duplicate last hash if odd number of hashes
        if (hashes.size() % 2 != 0) {
            hashes.push_back(hashes.back());
        }

        // Create vector to store new hashes in
        std::vector<std::string> new_hashes;

        // Concatenate each pair of hashes
        for (size_t i = 0; i < hashes.size(); i += 2) {
            new_hashes.push_back(signature::hash(hashes[i] + hashes[i + 1]));
        }

        // Replace old vector by new one
        hashes = new_hashes;
    }

    merkleRoot_ = hashes[0];
}

std::string Header::getID() const {
    return signature::hash(prevBlockDigest_ + merkleRoot_);
}

std::string Header::to_string() {
    json j;
    j["prevBlockDigest"] = prevBlockDigest_;
    j["merkleRoot"] = merkleRoot_;
    return j.dump();
}

void Header::from_string(std::string header_string) {
    json j = json::parse(header_string);
    prevBlockDigest_ = j.at("prevBlockDigest");
    merkleRoot_ = j.at("merkleRoot");
}
