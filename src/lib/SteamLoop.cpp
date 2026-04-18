#include <iostream>
#include <thread>
#include <steam/steam_api_flat.h>
#include <steam/steam_gameserver.h>

#include "lib/SteamLoop.h"
#include "lib/DebugLog.h"

void SteamLoop::OnSteamConnectFailure(SteamServerConnectFailure_t* pParam) {
	DebugLog("OnSteamServersConnectFailure: %d\n", pParam->m_eResult);
}

void SteamLoop::OnSteamConnected(SteamServersConnected_t* pParam) {
	DebugLog("OnSteamServersConnected\n");
	if (this->callback) {
		this->callback();
	}
}

void SteamLoop::OnSteamDisconnected(SteamServersDisconnected_t* pParam) {
	DebugLog("OnSteamServersDisconnected\n");
}

SteamLoop::SteamLoop() : running(false) {
}

SteamLoop::~SteamLoop() {
	this->Stop();
}

void SteamLoop::RunLoop() {
	while (this->running) {
		SteamGameServer_RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool SteamLoop::Start(CallbackSteamConnected callback) {
	if (!this->running) {
		DebugLog("SteamLoop is already running\n");
		return false;
	}

	this->running = true;
	this->callback = callback;

	bool result = SteamGameServer_Init(
        0,
        27015,
        27016,
        eServerModeNoAuthentication,
        "1.0.0.0");
	if (!result) {
		DebugLog("SteamGameServer_Init failed\n");
		return false;
	}

	SteamGameServer()->LogOnAnonymous();

	this->runThread = std::thread(&SteamLoop::RunLoop, this);
	return result;
}


bool SteamLoop::Stop() {
	this->running = false;
	if (this->runThread.joinable()) {
		this->runThread.join();
	}
	SteamGameServer_Shutdown();
	return true;
}