#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <steam/steam_api_flat.h>

#define SHA512_SIZE 128
#define APP_VERSION 1

struct user_info {
	SteamNetworkingIdentity identity;
	uint32_t ip;
	std::chrono::time_point<std::chrono::system_clock> lastPingTime;
};

#endif // CONSTANTS_H
