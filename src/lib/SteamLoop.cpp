#include <iostream>
#include <thread>
#include <steam/steam_api_flat.h>

#include "lib/SteamLoop.h"
#include "lib/DebugLog.h"

SteamLoop::SteamLoop() : running(false) {
}
SteamLoop::~SteamLoop() {
	Stop();
}
void SteamLoop::RunLoop() {
	this->running = true;
	while (this->running) {
		SteamAPI_RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
void SteamLoop::Stop() {
	this->running = false;
	if (this->runThread.joinable()) {
		this->runThread.join();
	}
	SteamAPI_Shutdown();
}

bool SteamLoop::Start() {
	bool result = SteamAPI_IsSteamRunning();
	if (!result) {
		DebugLog("SteamAPI_IsSteamRunning failed\n");
		return false;
	}
	result = SteamAPI_Init();
	if (!result) {
		DebugLog("SteamAPI_Init failed\n");
		return false;
	}
	if (!this->running) {
		this->runThread = std::thread(&SteamLoop::RunLoop, this);
	}
	return result;
}
