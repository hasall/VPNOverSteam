#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include "lib/IpPool.h"
#include "lib/Utils.h"
#include "lib/DebugLog.h"

#pragma comment(lib, "Ws2_32.lib")

IpPool::IpPool(std::string start, std::string end) : IpPool(Utils::FromString(start), Utils::FromString(end)) {}
IpPool::IpPool(uint32_t start, uint32_t end) {
    this->start_ = ntohl(start);
    this->end_ = ntohl(end);
    DebugLog("Create IP pool from %u to %u\n", this->start_, this->end_);
    if (this->start_ > this->end_)
        throw std::invalid_argument("Invalid IP range");

    for (uint32_t ip = this->start_; ip <= this->end_; ++ip) {
        this->free_.push(ip);
    }
}

uint32_t IpPool::Allocate() {
    if (this->free_.empty())
        throw std::runtime_error("No available IPs");

    uint32_t ip = this->free_.front();
    this->free_.pop();
    this->used_.insert(ip);
    return htonl(ip);
}

void IpPool::Release(uint32_t ip) {
	ip = ntohl(ip);
    if (ip < this->start_ || ip > this->end_)
        throw std::out_of_range("IP out of pool range");

    auto it = this->used_.find(ip);
    if (it == this->used_.end())
        throw std::invalid_argument("IP not allocated");

    this->used_.erase(it);
    this->free_.push(ip);
}

size_t IpPool::Available() const {
    return this->free_.size();
}

size_t IpPool::Used() const {
    return this->used_.size();
}
