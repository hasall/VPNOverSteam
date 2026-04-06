#include <string>
#include <cstdint>
#include <queue>
#include <unordered_set>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>

#ifdef _WIN32
    #include <bcrypt.h>
    #pragma comment(lib, "bcrypt.lib")
#else
    #include <arpa/inet.h>
    #include <openssl/sha.h>
#endif

#include "lib/Utils.h"
#include "lib/DebugLog.h"



std::string Utils::SHA512(const std::string& str) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    NTSTATUS status;
    DWORD cbData = 0, cbHash = 0;
    PBYTE pbHashObject = NULL;
    PBYTE pbHash = NULL;
    std::string result;

    // Open algorithm provider
    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA512_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status)) {
        return "";
    }

    // Get hash object size
    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbData, sizeof(DWORD), &cbData, 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    pbHashObject = (PBYTE)malloc(cbData);
    if (pbHashObject == NULL) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // Get hash size
    status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0);
    if (!BCRYPT_SUCCESS(status)) {
        free(pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    pbHash = (PBYTE)malloc(cbHash);
    if (pbHash == NULL) {
        free(pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // Create hash object
    status = BCryptCreateHash(hAlg, &hHash, pbHashObject, cbData, NULL, 0, 0);
    if (!BCRYPT_SUCCESS(status)) {
        free(pbHash);
        free(pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // Hash the data
    status = BCryptHashData(hHash, (PBYTE)str.c_str(), (ULONG)str.size(), 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptDestroyHash(hHash);
        free(pbHash);
        free(pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // Get the hash
    status = BCryptFinishHash(hHash, pbHash, cbHash, 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptDestroyHash(hHash);
        free(pbHash);
        free(pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // Convert to hex string
    std::stringstream ss;
    for (DWORD i = 0; i < cbHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)pbHash[i];
    }
    result = ss.str();

    // Cleanup
    BCryptDestroyHash(hHash);
    free(pbHash);
    free(pbHashObject);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    return result;
#else
    unsigned char hash[SHA512_DIGEST_LENGTH];
    ::SHA512((const unsigned char*)str.c_str(), str.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
#endif
}

// expect ip in network byte order
std::string Utils::ToString(uint32_t ip) {
    return
        std::to_string(ip & 0xFF) + "."
        + std::to_string((ip >> 8) & 0xFF) + "."
        + std::to_string((ip >> 16) & 0xFF) + "."
        + std::to_string((ip >> 24) & 0xFF);
}

// return ip in network byte order
uint32_t Utils::FromString(const std::string& ip) {
    uint32_t result = 0;
    int parts = 0;

    size_t i = 0;
    while (i < ip.size()) {
        if (parts >= 4) throw std::invalid_argument("Too many parts");

        if (!isdigit(ip[i])) throw std::invalid_argument("Invalid character");

        uint32_t value = 0;
        size_t start = i;

        while (i < ip.size() && isdigit(ip[i])) {
            value = value * 10 + (ip[i] - '0');
            if (value > 255) throw std::out_of_range("Octet > 255");
            i++;
        }

        if (i == start) throw std::invalid_argument("Empty octet");

        result = (result << 8) | value;
        parts++;

        if (i < ip.size()) {
            if (ip[i] != '.') throw std::invalid_argument("Expected dot");
            i++;
        }
    }

    if (parts != 4) throw std::invalid_argument("Invalid IPv4");

    return htonl(result);
}

void Utils::PrintBytes(const char* message, size_t size) {
    DebugLogArr(message, size);
}