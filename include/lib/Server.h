#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <map>
#include <string>

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
#include "LobbyServerController.h"
#include "SteamMessageProcessor.h"

class Server {
public:
#ifdef _WIN32
	Server(GUID guid);
#else
	Server();
#endif
	~Server();

	void JoinMember(uint64 userId, const char* userName, uint32_t ip);
	void LeftMember(uint64 userId);

	void Start(std::string password);
	void Stop();

private:
	std::string password;
	std::map<uint32_t, SteamNetworkingIdentity> ipToClient;
	IpPool ipPool;
	SteamMessageProcessor steamMessageProcessor;

#ifdef _TUN_MOCK_ENABLED
	TUN<TUNMock> tunMessageProcessor;
#else
#ifdef _WIN32
	TUN<TUNWindows> tunMessageProcessor;
#else
	TUN<TUNLinux> tunMessageProcessor;
#endif // _WIN32
#endif // _TUN_MOCK_ENABLED

	void TUNDataReceiver(const char* message, size_t size);
	void SteamMessageReceiver(CSteamID userId, const char* message, size_t size);
	void SteamSystemMessageReceiver(CSteamID userId, const char* message, size_t size);

	void SendHandshakeMassage(SteamNetworkingIdentity user, uint32_t ip);
	void SendErrorMassage(SteamNetworkingIdentity user, uint32_t errorCode);
};

#endif // SERVER_H
