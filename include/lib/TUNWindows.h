#ifndef TUN_WINDOWS_H
#define TUN_WINDOWS_H

#include <wintun.h>
#undef max

#include <functional>
#include <string>
#include <thread>
//#include <windows.h>

#include "TUN.h"

class TUNWindows {
public:
    TUNWindows(TUNMessageReceiver receiver, GUID guid);
    void Start(uint32_t ip);
    void Stop();
    void SendData(const char* message, size_t size);

private:
    TUNMessageReceiver receiver;

	bool running;
	std::thread receiverThread;

	void Receiver();

    HMODULE Wintun; 
    GUID Guid;
    WINTUN_ADAPTER_HANDLE Adapter;
    WINTUN_SESSION_HANDLE Session;

    static HMODULE InitializeWintun();
	void CreateAdapter();
	void SetupAdapter(uint32_t ip);
	void SetupSession();
	void CleanupAdapter();
};

#endif // TUN_WINDOWS_H
