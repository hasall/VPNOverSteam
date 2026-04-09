#include <string>
#include <cstdint>
#include <queue>
#include <unordered_set>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>

#include <openssl/sha.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <arpa/inet.h>
#endif

#include "lib/Utils.h"
#include "lib/DebugLog.h"



std::string Utils::SHA512(const std::string& str) {
    unsigned char hash[SHA512_DIGEST_LENGTH];
    ::SHA512((const unsigned char*)str.c_str(), str.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
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