#include <iostream>
#include <thread>
#include <steam/steam_api_flat.h>
#include <steam/steam_gameserver.h>

#include "lib/SteamLoop.h"
#include "lib/DebugLog.h"

void SteamLoop::OnSteamServersConnectFailure(SteamServerConnectFailure_t* pParam) {
	DebugLog("OnSteamServersConnectFailure: %d\n", pParam->m_eResult);
}

void SteamLoop::OnSteamServersConnected(SteamServersConnected_t* pParam) {
	DebugLog("OnSteamServersConnected\n");
	if (this->callback) {
		this->callback();
	}
}

void SteamLoop::OnSteamServersDisconnected(SteamServersDisconnected_t* pParam) {
	DebugLog("OnSteamServersDisconnected\n");
}

SteamLoop::SteamLoop() : running(false) {
}

SteamLoop::~SteamLoop() {
	this->StopServer();
}
void SteamLoop::RunLoop() {
	this->running = true;
	while (this->running) {
		SteamAPI_RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void SteamLoop::RunServerLoop() {
	this->running = true;
	while (this->running) {
		SteamGameServer_RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool SteamLoop::StartServer(CallbackSteamConnected callback) {
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

	if (!this->running) {
		this->runThread = std::thread(&SteamLoop::RunServerLoop, this);
	}
	return result;
}


bool SteamLoop::StopServer() {
	this->running = false;
	if (this->runThread.joinable()) {
		this->runThread.join();
	}
	SteamGameServer_Shutdown();
	return true;
}