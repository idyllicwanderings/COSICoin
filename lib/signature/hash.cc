#include "signature/hash.h"

using namespace signature;

std::string signature::hash(std::string data) {
    // create a new hashing object
    SHA256 sha256;
    // Calculate hash
    return sha256(data);
}

// @overload
unsigned char* signature::hash(unsigned char* data) {
    SHA256 sha256;
    std::string datastr;
    datastr = (char*)data;

    return (unsigned char*)sha256(datastr).data();
}

// @overload
unsigned char signature::hash(unsigned char data) {
    SHA256 sha256;
    std::string datastr;
    datastr = (char)data;

    return (unsigned char)sha256(datastr).data()[0];
}

int Signature::getRandomBytes(unsigned char* buf, int length, int block) {
    int fd, sofar, rc;

    fd = open((block) ? "/dev/random" : "/dev/urandom", O_RDONLY, 0666);
    if (fd < 0) {
        return -1;
    }

    sofar = 0;
    do {
        rc = (int)read(fd, &buf[sofar], length);
        if (rc < 0) {
            close(fd);
            return -2;
        }
        length -= rc;
        sofar += rc;
    } while (length > 0);

    close(fd);

    return 0;
}

int Signature::KeyGen() {
    // generate using /dev/urandom
    if (!getRandomBytes(private_key_.S0, sizeof(private_key_.S0), 0)) {
        std::cout << " S0: RNG failed!" << std::endl;
        return 0;
    }

    if (!getRandomBytes(private_key_.S1, sizeof(private_key_.S1), 0)) {
        std::cout << " S1: RNG failed!" << std::endl;
        return 0;
    }

    for (int i = 0; i < KEY_LEN_; i++) {
        public_key_.S0[i] = hash(private_key_.S0[i]);
        public_key_.S1[i] = hash(private_key_.S1[i]);
    }

    // TODO: test
    return 1;
}

int Signature::Sign(unsigned char m, unsigned char* sig) {
    int m_binary[KEY_LEN_];

    Char2Binary(m, m_binary);

    for (int i = 0; i < KEY_LEN_; i++) {
        sig[i] = (m_binary[i]) ? private_key_.S1[i] : private_key_.S1[i];
    }

    return 1;
}

int signature::Verify(unsigned char m, unsigned char* sig, SigKey public_key) {
    int m_binary[KEY_LEN_];

    Char2Binary(m, m_binary);

    for (int i = 0; i < KEY_LEN_; i++) {
        unsigned char pk_i = (m_binary[i]) ? public_key.S1[i] : public_key.S0[i];
        if (pk_i != sig[i]) {
            return 0;
        }
    }

    return 1;
}