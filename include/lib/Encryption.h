#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <cstdint>
#include <string>
#include <vector>

class Encryption {
public:
    Encryption();
    ~Encryption();

    void SetKey(const std::string& passwordHash);
    std::vector<uint8_t> Encrypt(const char* data, size_t size);
    std::vector<uint8_t> Decrypt(const char* data, size_t size);

private:
    void DeriveKey(const std::string& passwordHash);

    bool keySet = false;
    bool enabled = false;
    std::vector<uint8_t> key;
    uint64_t sendCounter = 0;

    static constexpr size_t KeySize = 32;
    static constexpr size_t NonceSize = 12;
    static constexpr size_t TagSize = 16;
};

#endif // ENCRYPTION_H
