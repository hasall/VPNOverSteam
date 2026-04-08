#ifndef STEAM_LOOP_H
#define STEAM_LOOP_H

#include <iostream>
#include <thread>
#include <vector>
#include <functional>

#include <steam/steam_api_flat.h>

#define CallbackSteamConnected std::function<void()>


class SteamLoop {
public:
	SteamLoop();
	~SteamLoop();
	
	bool StartServer(CallbackSteamConnected callback);
	bool StopServer();

private:
	bool running;
	std::thread runThread;

	CallbackSteamConnected callback;

	void RunLoop();
	void RunServerLoop();

	STEAM_GAMESERVER_CALLBACK(SteamLoop, OnSteamServersConnectFailure, SteamServerConnectFailure_t);
	STEAM_GAMESERVER_CALLBACK(SteamLoop, OnSteamServersConnected, SteamServersConnected_t);
	STEAM_GAMESERVER_CALLBACK(SteamLoop, OnSteamServersDisconnected, SteamServersDisconnected_t);
};

#endif // STEAM_LOOP_H
