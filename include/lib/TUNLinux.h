#ifndef TUN_LINUX_H
#define TUN_LINUX_H

#include <functional>
#include <cstdint>
#include <thread>
#include <atomic>

#include "TUNCallback.h"

class TUNLinux {
public:
    TUNLinux(TUNMessageReceiver receiver);
    void Start(uint32_t ip);
    void Stop();
    void SendData(const char* message, size_t size);

private:
    TUNMessageReceiver receiver;

    std::atomic<bool> running;
    std::thread receiverThread;
    void Receiver();

    int tunFd;
    int InitializeTun();
    void SetupTun(uint32_t ip);
    void Cleanup();
};

#endif // TUN_LINUX_H
