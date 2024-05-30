#include "signature/hash.h"

using namespace signature;

std::string signature::bitset_to_string(std::bitset<KEY_LEN_> bits) {
    std::string str;
    for (int i = 0; i < KEY_LEN_; i += 8) {
        unsigned char byte = 0;
        for (int j = 0; j < 8; j++) {
            byte |= (bits[i + j] << j);
        }
        str.push_back(byte);
    }
    return str;
}

std::bitset<KEY_LEN_> signature::string_to_bitset(std::string str) {
    std::bitset<KEY_LEN_> bits;
    int index = 0;
    for (char c : str) {
        unsigned char byte = static_cast<unsigned char>(c);
        for (int j = 0; j < 8; j++) {
            bits[index++] = ((byte >> j) & 1);
        }
    }
    return bits;
}

std::string signature::signature_to_json(std::vector<std::string> signature) {
    json j;
    for (std::string sig : signature) {
        j.push_back(string_to_bitset(sig).to_string());
    }
    return j.dump();
}

std::vector<std::string> signature::json_to_string(std::string string) {
    json j = json::parse(string);
    std::vector<std::string> sig;
    for (std::string str : j) {
        sig.push_back(bitset_to_string(std::bitset<KEY_LEN_>(str)));
    }
    return sig;
}

std::string signature::hash(std::string data) {
    // create a new hashing object
    SHA256 sha256;
    // Calculate hash
    return sha256(data);
}

std::bitset<KEY_LEN_> Signature::getRandomBits() {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("Failed to open /dev/urandom");
    }

    std::bitset<KEY_LEN_> buf;

    int sofar = 0;
    while (sofar < KEY_LEN_) {
        unsigned char temp_buf;
        int rc = (int)read(fd, &temp_buf, 1);
        if (rc < 0) {
            close(fd);
            throw std::runtime_error("Failed to read from /dev/urandom");
        }
        for (int bit = 0; bit < 8 && sofar < KEY_LEN_; bit++, sofar++) {
            buf[sofar] = (temp_buf >> bit) & 1;
        }
    }

    close(fd);

    return buf;
}

int Signature::KeyGen() {
    private_key_.S0.clear();
    private_key_.S1.clear();
    public_key_.S0.clear();
    public_key_.S1.clear();
    for (int i = 0; i < KEY_LEN_; i++) {
        std::string K0 = bitset_to_string(getRandomBits());
        std::string K1 = bitset_to_string(getRandomBits());
        private_key_.S0.push_back(K0);
        private_key_.S1.push_back(K1);
        public_key_.S0.push_back(hash(K0));
        public_key_.S1.push_back(hash(K1));
    }

    return 1;
}

std::vector<std::string> signature::Sign(std::string m, SigKey private_key) {
    std::vector<std::string> sig;

    // Check if key is generated & is right length
    if (private_key.S0.size() != KEY_LEN_ || private_key.S1.size() != KEY_LEN_) {
        return sig;
    }

    // Crop message to max KEY_LEN_/8 chars
    if (m.size() > KEY_LEN_ / 8) {
        m = m.substr(0, KEY_LEN_ / 8);
    }

    // Convert message to bits & append 0 if too short
    std::bitset<KEY_LEN_> m_bits = string_to_bitset(m);

    // Build signature
    for (int i = 0; i < KEY_LEN_; i++) {
        sig.push_back(m_bits[i] ? private_key.S1[i] : private_key.S0[i]);
    }

    return sig;
}

int signature::Verify(std::string m, std::vector<std::string> sig, SigKey public_key) {
    // Check if key & sig has correct length
    if (public_key.S0.size() != KEY_LEN_ || public_key.S1.size() != KEY_LEN_ || sig.size() != KEY_LEN_) {
        return -1;
    }

    // Crop message to max KEY_LEN_/8 chars
    if (m.size() > KEY_LEN_ / 8) {
        m = m.substr(0, KEY_LEN_ / 8);
    }

    // Convert message to bits & append 0 if too short
    std::bitset<KEY_LEN_> m_bits = string_to_bitset(m);

    // Check signature
    for (int i = 0; i < KEY_LEN_; i++) {
        std::string k_i = m_bits[i] ? public_key.S1[i] : public_key.S0[i];
        if (k_i != hash(sig[i])) {
            return 0;
        }
    }
    return 1;
}

std::string signature::SigKey_to_string(SigKey sigkey) {
    json j;

    if (sigkey.S0.size() != KEY_LEN_ || sigkey.S1.size() != KEY_LEN_) {
        throw std::runtime_error("Cannot convert SigKey to string, invalid signature key length (S0: " + std::to_string(sigkey.S0.size()) + ", S1: " + std::to_string(sigkey.S1.size()) + ")");
    }

    for (int i = 0; i < KEY_LEN_; i++) {
        j["S0"].push_back(sigkey.S0[i]);
        j["S1"].push_back(sigkey.S1[i]);
    }

    return j.dump();
}

SigKey signature::SigKey_from_string(std::string sigkey_string) {
    json j = json::parse(sigkey_string);
    SigKey sigkey;

    if (j.at("S0").size() != KEY_LEN_ || j.at("S0").size() != KEY_LEN_) {
        throw std::runtime_error("Cannot convert string to SigKey, invalid signature key length (S0: " + std::to_string(sigkey.S0.size()) + ", S1: " + std::to_string(sigkey.S1.size()) + ")");
    }

    for (int i = 0; i < KEY_LEN_; i++) {
        sigkey.S0.push_back(j.at("S0")[i]);
        sigkey.S1.push_back(j.at("S1")[i]);
    }
    return sigkey;
}

std::string signature::print_SigKey(SigKey sigkey) {
    return "S0: " + sigkey.S0[0].substr(0, 10) + "... S1: " + sigkey.S1[0].substr(0, 10) + "...";
}
