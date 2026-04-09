#ifndef CLIENT_H
#define CLIENT_H

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <string>
#include <mutex>

#include <steam/steam_api_flat.h>

#include "TUN.h"

#ifdef _TUN_MOCK_ENABLED
#include "TUNMock.h"
#else
#ifdef _WIN32
#include "TUNWindows.h"
#else
#include "TUNLinux.h"
#endif // _WIN32
#endif // _TUN_MOCK_ENABLED

#include "IpPool.h"
#include "SteamMessageProcessor.h"

#define ReceiveNewIpCallback std::function<void(uint32_t ip)>

class Client {
public:
#ifdef _WIN32
	Client(ReceiveNewIpCallback callback, GUID guid);
#else
	Client(ReceiveNewIpCallback callback);
#endif
	~Client();

	void Start(uint64 serverUserId, std::string password);
	void Stop();

private:
	std::string password;
	std::map<uint32_t, SteamNetworkingIdentity> ipToClient;
	SteamMessageProcessor steamMessageProcessor;

	// Block all messages from TUN before handshake
	std::mutex mtx;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock;
	bool mtx_ready = false;

#ifdef _TUN_MOCK_ENABLED
	TUN<TUNMock> tunMessageProcessor;
#else
#ifdef _WIN32
	TUN<TUNWindows> tunMessageProcessor;
#else
	TUN<TUNLinux> tunMessageProcessor;
#endif // _WIN32
#endif // _TUN_MOCK_ENABLED

	ReceiveNewIpCallback callback;

	void JoinMember(uint64 userId, uint32_t ip);
	void LeftMember(uint64 userId);

	void TUNDataReceiver(const char* message, size_t size);
	void SteamMessageReceiver(CSteamID userId, const char* message, size_t size);
	void SteamSystemMessageReceiver(CSteamID userId, const char* message, size_t size);

	void SendPasswordMassage(SteamNetworkingIdentity user);
	void SendErrorMassage(SteamNetworkingIdentity user, uint32_t errorCode);
};

#endif // CLIENT_H
