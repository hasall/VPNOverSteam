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
#include "IpPool.h"
#include "SteamMessageProcessor.h"
#include "Constants.h"

#define ReceiveNewIpCallback std::function<void(uint32_t ip)>

class Client {
public:
	Client(ReceiveNewIpCallback callback);
	~Client();

	void Start(uint64 serverUserId, std::string password);
	void Stop();

private:
	std::string password;
	std::vector<user_info> usersList;
	SteamMessageProcessor steamMessageProcessor;

	// Block all messages from TUN before handshake
	std::mutex mtx;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock;
	bool mtx_ready = false;

	TUN tunMessageProcessor;

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
