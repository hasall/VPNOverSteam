#include "lib/Encryption.h"

#include <cstring>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdexcept>

Encryption::Encryption() {
}

Encryption::~Encryption() = default;

void Encryption::SetKey(const std::string& passwordHash) {
    this->DeriveKey(passwordHash);
    this->enabled = true;
    this->keySet = true;
}

void Encryption::DeriveKey(const std::string& passwordHash) {
    this->key.assign(KeySize, 0);

    unsigned int digestLen = 0;
    if (EVP_Digest(passwordHash.data(), passwordHash.size(), this->key.data(), &digestLen, EVP_sha256(), nullptr) != 1 || digestLen != KeySize) {
        throw std::runtime_error("Failed to derive encryption key");
    }
}

std::vector<uint8_t> Encryption::Encrypt(const char* data, size_t size) {
    if (!this->enabled || !this->keySet) {
        return std::vector<uint8_t>(data, data + size);
    }

    std::vector<uint8_t> output(NonceSize + size + TagSize);
    uint8_t nonce[NonceSize] = {};

    uint64_t counter = this->sendCounter++;
    for (int i = 0; i < 8; ++i) {
        nonce[NonceSize - 1 - i] = static_cast<uint8_t>((counter >> (8 * i)) & 0xFF);
    }

    std::memcpy(output.data(), nonce, NonceSize);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        throw std::runtime_error("Failed to create cipher context");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize ChaCha20-Poly1305 encryptor");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, NonceSize, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set nonce length for ChaCha20-Poly1305");
    }

    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, this->key.data(), nonce) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set ChaCha20-Poly1305 key/nonce");
    }

    int outlen = 0;
    if (EVP_EncryptUpdate(ctx, output.data() + NonceSize, &outlen, reinterpret_cast<const unsigned char*>(data), static_cast<int>(size)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("ChaCha20-Poly1305 encryption failed");
    }

    int tmplen = 0;
    if (EVP_EncryptFinal_ex(ctx, output.data() + NonceSize + outlen, &tmplen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("ChaCha20-Poly1305 finalization failed");
    }
    outlen += tmplen;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, TagSize, output.data() + NonceSize + outlen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to retrieve ChaCha20-Poly1305 tag");
    }

    EVP_CIPHER_CTX_free(ctx);
    output.resize(NonceSize + outlen + TagSize);
    return output;
}

std::vector<uint8_t> Encryption::Decrypt(const char* data, size_t size) {
    if (!this->enabled || !this->keySet) {
        return std::vector<uint8_t>(data, data + size);
    }

    if (size < NonceSize + TagSize) {
        throw std::runtime_error("Encrypted message too small");
    }

    const uint8_t* nonce = reinterpret_cast<const uint8_t*>(data);
    const uint8_t* ciphertext = reinterpret_cast<const uint8_t*>(data + NonceSize);
    size_t ciphertextLen = size - NonceSize - TagSize;
    const uint8_t* tag = reinterpret_cast<const uint8_t*>(data + NonceSize + ciphertextLen);

    std::vector<uint8_t> output(ciphertextLen);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        throw std::runtime_error("Failed to create cipher context");
    }

    if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize ChaCha20-Poly1305 decryptor");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, NonceSize, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set nonce length for ChaCha20-Poly1305");
    }

    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, this->key.data(), nonce) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set ChaCha20-Poly1305 key/nonce");
    }

    int outlen = 0;
    if (EVP_DecryptUpdate(ctx, output.data(), &outlen, ciphertext, static_cast<int>(ciphertextLen)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("ChaCha20-Poly1305 decryption failed");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, TagSize, const_cast<unsigned char*>(tag)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set ChaCha20-Poly1305 tag for verification");
    }

    int tmplen = 0;
    if (EVP_DecryptFinal_ex(ctx, output.data() + outlen, &tmplen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("ChaCha20-Poly1305 authentication failed");
    }
    outlen += tmplen;

    EVP_CIPHER_CTX_free(ctx);
    output.resize(outlen);
    return output;
}
