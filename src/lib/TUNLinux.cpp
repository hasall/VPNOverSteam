#ifndef _WIN32

#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include "lib/TUNLinux.h"
#include "lib/DebugLog.h"

TUNLinux::TUNLinux(TUNMessageReceiver receiver) : receiver(receiver) {
	DebugLog("TUNMock selected\n");
    this->tunFd = this->Initialize();
}

void TUNLinux::Start(uint32_t ip) {
    if (!this->running) {
        this->receiverThread = std::thread(&TUNLinux::Receiver, this);
    }
}

void TUNLinux::Stop() {
    this->running = false;
    if (this->receiverThread.joinable()) {
        this->receiverThread.join();
    }
    this->Cleanup();
}

void TUNLinux::SendData(const char* message, size_t size) {
    if (this->tunFd >= 0) {
        write(this->tunFd, message, size);
    }
}

void TUNLinux::Receiver() {
    this->running = true;

    fd_set readfds;
    char buffer[1500];

    while (this->running) {
        FD_ZERO(&readfds);
        FD_SET(this->tunFd, &readfds);

        int maxfd = this->tunFd + 1;
        int ret = select(maxfd, &readfds, NULL, NULL, NULL);

        if (ret > 0) {
            if (FD_ISSET(this->tunFd, &readfds)) {
                int len = read(this->tunFd, buffer, sizeof(buffer));
                if (len > 0)
                    this->receiver(buffer, len);
            }
        }
    }
}

int TUNLinux::Initialize() {
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if (ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
        perror("ioctl");
        this->Cleanup();
        return -1;
    }

    return fd;
}

void TUNLinux::Cleanup() {
    if (this->tunFd >= 0) {
        close(this->tunFd);
        this->tunFd = -1;
    }
}

#endif
