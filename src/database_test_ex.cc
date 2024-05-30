#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "blockchain/block.h"
#include "blockchain/header.h"
#include "json/json.hpp"

using json = nlohmann::json;

void to_json(json& j) {
    blockchain::Header header("header1");
    j["prevBlockDigest"] = header.getPrevBlockDigest();
    j["merkleRoot"] = header.getMerkleRoot();
}

int main() {
    json j;
    to_json(j);
    std::cout << j << std::endl;
    return 1;
}