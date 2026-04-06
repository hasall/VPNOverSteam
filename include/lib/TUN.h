#ifndef TUN_H
#define TUN_H

#include <functional>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif


#define TUNMessageReceiver std::function<void(const char* message, size_t size)>

template<typename T>
class TUN {
public:

#ifdef _WIN32
	TUN(TUNMessageReceiver receiver, GUID guid) : impl(receiver, guid) {}
#else
	TUN(TUNMessageReceiver receiver) : impl(receiver) {}
#endif

	void Start(uint32_t ip) { impl.Start(ip); }
	void Stop() { impl.Stop(); }
	void SendData(const char* message, size_t size) { impl.SendData(message, size); }

private:
	T impl;
};

#endif // TUN_H
