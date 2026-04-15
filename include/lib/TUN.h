#ifndef TUN_H
#define TUN_H

#include <functional>

#ifdef _TUN_MOCK_ENABLED

#include "TUNMock.h"

#else

#ifdef _WIN32

#include "TUNWindows.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#else

#include "TUNLinux.h"

#endif // _WIN32
#endif // _TUN_MOCK_ENABLED

#include "TUNCallback.h"
#include "Config.h"

class TUN {
public:

	TUN(TUNMessageReceiver receiver) : impl(
		receiver
		#ifdef _WIN32
		, Config::AdapterGuid
		#endif
	) {}

	void Start(uint32_t ip) { this->impl.Start(ip); }
	void Stop() { this->impl.Stop(); }
	void SendData(const char* message, size_t size) { this->impl.SendData(message, size); }

private:

#ifdef _TUN_MOCK_ENABLED
	TUNMock impl;
#else
#ifdef _WIN32
	TUNWindows impl;
#else
	TUNLinux impl;
#endif // _WIN32
#endif // _TUN_MOCK_ENABLED

};

#endif // TUN_H
