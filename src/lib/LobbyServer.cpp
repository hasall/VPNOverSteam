#include "lib/LobbyServer.h"
#include "lib/Constants.h"
#include "lib/DebugLog.h"
#include "lib/Utils.h"
#include "lib/Config.h"

LobbyServer::LobbyServer() :
	m_CallbackLobbyCreated(this, &LobbyServer::OnLobbyCreated),
	m_CallbackLobbyEnter(this, &LobbyServer::OnLobbyEnter) {}

void LobbyServer::CreateLobby(
	std::string name,
	std::string password,
	CallbackCreateLobby callback
) {
	this->callbackCreateLobby = callback;
	this->lobbyName = name;
	this->password = Utils::SHA512(name + ":" + password);
	SteamAPI_ISteamMatchmaking_CreateLobby(SteamMatchmaking(), k_ELobbyTypePublic, 32);
}

void LobbyServer::OnLobbyCreated(LobbyCreated_t* pCallback) {

	if (pCallback->m_eResult != k_EResultOK) {
		DebugLog("LobbyServer::OnLobbyCreated Error\n");
		if (this->callbackCreateLobby != NULL) {
			this->callbackCreateLobby(pCallback->m_eResult, nullptr);
		}
		return;
	}
}

void LobbyServer::OnLobbyEnter(LobbyEnter_t* pCallback) {
	DebugLog("LobbyServer::OnLobbyEnter\n");

	if (pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
		DebugLog("LobbyServer::OnLobbyEnter Error\n");
		if (this->callbackCreateLobby != NULL) {
			this->callbackCreateLobby(pCallback->m_EChatRoomEnterResponse, nullptr);
		}
		return;
	}

	SteamAPI_ISteamMatchmaking_SetLobbyData(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, Config::LobbyGameNameKey.c_str(), Config::LobbyGameNameValue.c_str());
	SteamAPI_ISteamMatchmaking_SetLobbyData(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, Config::LobbyNameKey.c_str(), this->lobbyName.c_str());
	SteamAPI_ISteamMatchmaking_SetLobbyData(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, Config::LobbyPasswordKey.c_str(), this->password.c_str());

	SteamAPI_ISteamMatchmaking_SetLobbyMemberData(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, Config::LobbyUserNameKey.c_str(), SteamAPI_ISteamFriends_GetPersonaName(SteamFriends()));
	SteamAPI_ISteamMatchmaking_SetLobbyMemberData(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, Config::LobbyUserIpKey.c_str(), Config::ServerIp.c_str());

	if (this->callbackCreateLobby != NULL) {
		this->callbackCreateLobby(pCallback->m_EChatRoomEnterResponse, new LobbyServerController(pCallback->m_ulSteamIDLobby));
	}
}
