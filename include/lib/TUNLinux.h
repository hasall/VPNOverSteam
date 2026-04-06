#ifndef TUN_LINUX_H
#define TUN_LINUX_H

#include <functional>
#include <cstdint>
#include <thread>

#include "TUN.h"

class TUNLinux {
public:
    TUNLinux(TUNMessageReceiver receiver);
    void Start(uint32_t ip);
    void Stop();
    void SendData(const char* message, size_t size);

private:
    TUNMessageReceiver receiver;

    bool running;
    std::thread receiverThread;
    void Receiver();

    int tunFd;
    int Initialize();
    void Cleanup();
};

#endif // TUN_LINUX_H
