#ifndef IPPOOL_H
#define IPPOOL_H

#include <cstdint>
#include <queue>
#include <unordered_set>
#include <stdexcept>
#include <string>
#include <sstream>

class IpPool {
private:
    uint32_t start_;
    uint32_t end_;

    std::queue<uint32_t> free_;
    std::unordered_set<uint32_t> used_;

public:
    IpPool(std::string start, std::string end);
    IpPool(uint32_t start, uint32_t end);

    uint32_t Allocate();
    void Release(uint32_t ip);

    size_t Available() const;
    size_t Used() const;
};

#endif // IPPOOL_H
