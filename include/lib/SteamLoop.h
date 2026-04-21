#ifndef STEAM_LOOP_H
#define STEAM_LOOP_H

#include <iostream>
#include <thread>
#include <vector>
#include <functional>
#include <atomic>

#include <steam/steam_api_flat.h>

#define CallbackSteamConnected std::function<void()>


class SteamLoop {
public:
	SteamLoop();
	~SteamLoop();
	
	bool Start(CallbackSteamConnected callback);
	bool Stop();

private:
	std::atomic<bool> running = false;
	std::thread runThread;

	CallbackSteamConnected callback;

	void RunLoop();

	STEAM_GAMESERVER_CALLBACK(SteamLoop, OnSteamConnectFailure, SteamServerConnectFailure_t);
	STEAM_GAMESERVER_CALLBACK(SteamLoop, OnSteamConnected, SteamServersConnected_t);
	STEAM_GAMESERVER_CALLBACK(SteamLoop, OnSteamDisconnected, SteamServersDisconnected_t);
};

#endif // STEAM_LOOP_H
