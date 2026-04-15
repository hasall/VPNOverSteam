#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <map>
#include <string>

#include <steam/steam_api_flat.h>

#include "TUN.h"
#include "IpPool.h"
#include "SteamMessageProcessor.h"

class Server {
public:
	Server();
	~Server();

	void Start(std::string password);
	void Stop();

private:
	std::string password;
	std::map<uint32_t, SteamNetworkingIdentity> ipToClient;
	IpPool ipPool;
	SteamMessageProcessor steamMessageProcessor;

	TUN tunMessageProcessor;

	void JoinMember(uint64 userId, uint32_t ip);
	void LeftMember(uint64 userId);

	void TUNDataReceiver(const char* message, size_t size);
	void SteamMessageReceiver(CSteamID userId, const char* message, size_t size);
	void SteamSystemMessageReceiver(CSteamID userId, const char* message, size_t size);

	void SendHandshakeMassage(SteamNetworkingIdentity user, uint32_t ip);
	void SendErrorMassage(SteamNetworkingIdentity user, uint32_t errorCode);
	void SendNotifyOfNewMemberMessage(SteamNetworkingIdentity user, uint64 userID, uint32_t ip);
};

#endif // SERVER_H
