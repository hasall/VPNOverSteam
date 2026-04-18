#ifndef TUN_MOCK_H
#define TUN_MOCK_H

#include <thread>
#include <functional>
#include <atomic>

#include "TUN.h"

#define TUNMessageReceiver std::function<void(const char* message, size_t size)>

class TUNMock {
public:
	TUNMock(TUNMessageReceiver receiver);

	void Start();
	void Stop();
	void SetIp(uint32_t ip) {};

	void SendData(const char* message, size_t size);

	TUNMessageReceiver receiver;

	std::atomic<bool> running = false;
	std::thread runLoop = {};

	void Receiver(const char* message, size_t size);

	TUNMessageReceiver testCallback;
};

TUNMock* getMock();

#endif // TUN_MOCK_H
