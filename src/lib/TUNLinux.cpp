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
#include "lib/Utils.h"
#include "lib/Config.h"

TUNLinux::TUNLinux(TUNMessageReceiver receiver) : receiver(receiver), running(false) {
	DebugLog("TUNLinux selected\n");
    this->tunFd = this->InitializeTun();
}

void TUNLinux::Start(uint32_t ip) {
    if (this->running == false) {
        this->running = true;
        this->SetupTun(ip);
        this->receiverThread = std::thread(&TUNLinux::Receiver, this);
    }
    else {
        DebugLog("TUNLinux::Start: already running\n");
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
    fd_set readfds;
    char buffer[1500];

    while (this->running == true) {
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

int TUNLinux::InitializeTun() {
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    strncpy(ifr.ifr_name, Config::InterfaceName.c_str(), IFNAMSIZ - 1);

    if (ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
        perror("ioctl");
        this->Cleanup();
        return -1;
    }

    DebugLog("TUN interface created\n");
    DebugLogArr(ifr.ifr_name, IFNAMSIZ);

    return fd;
}

void TUNLinux::SetupTun(uint32_t ip) {
    // This function should set up the TUN interface with the given IP address.
    // The implementation can vary based on the system and requirements.
    // For example, you might use system calls or execute shell commands to configure the interface.
    // This is a placeholder for the actual implementation.

    // sudo ip addr add 10.0.0.1/24 dev tun0
    // sudo ip link set tun0 up
    // sudo sysctl -w net.ipv4.ip_forward=1

    std::string command = "ip addr add " + Utils::ToString(ip) + "/24 dev " + Config::InterfaceName;
    system(command.c_str());

    command = "ip link set dev " + Config::InterfaceName + " up";
    system(command.c_str());

    DebugLog("TUN interface configured with IP %s\n", Utils::ToString(ip).c_str());
}

void TUNLinux::Cleanup() {
    if (this->tunFd >= 0) {
        close(this->tunFd);
        this->tunFd = -1;
    }
}

#endif
