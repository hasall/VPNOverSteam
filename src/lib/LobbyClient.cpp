#include "lib/LobbyClient.h"
#include "lib/Constants.h"
#include "lib/DebugLog.h"
#include "lib/Utils.h"
#include "lib/Config.h"

LobbyClient::LobbyClient():
	m_CallbackLobbyMatchList(this, &LobbyClient::OnLobbyMatchList),
	m_CallbackLobbyEnter(this, &LobbyClient::OnLobbyEnter) {}

void LobbyClient::RequestLobbyList(CallbackRequestLobbyList callback) {
	DebugLog("LobbyClient::RequestLobbyList\n");
	this->callbackRequestLobbyList = callback;
	SteamAPI_ISteamMatchmaking_AddRequestLobbyListStringFilter(
		SteamMatchmaking(), 
		Config::LobbyGameNameKey.c_str(), 
		Config::LobbyGameNameValue.c_str(), 
		ELobbyComparison::k_ELobbyComparisonEqual
	);
	SteamAPI_ISteamMatchmaking_RequestLobbyList(SteamMatchmaking());
}

LobbyClient::~LobbyClient() {}

void LobbyClient::OnLobbyMatchList(LobbyMatchList_t* pCallback) {
	DebugLog("LobbyClient::OnLobbyMatchList found lobbies: %u\n", pCallback->m_nLobbiesMatching);

	if (pCallback->m_nLobbiesMatching == 0) {
		DebugLog("LobbyClient::OnLobbyMatchList error\n");
		if (this->callbackRequestLobbyList != nullptr) {
			this->callbackRequestLobbyList(1, nullptr, 0); // error
		}
		return;
	}

	LobbyList* lobbies = new LobbyList[pCallback->m_nLobbiesMatching];

	for (size_t i = 0; i < pCallback->m_nLobbiesMatching; i++) {
		uint64_steamid lobbyID = SteamAPI_ISteamMatchmaking_GetLobbyByIndex(SteamMatchmaking(), (int)i);
		lobbies[i].lobbyID = lobbyID;

        auto nameData = SteamAPI_ISteamMatchmaking_GetLobbyData(SteamMatchmaking(), lobbyID, Config::LobbyNameKey.c_str());
		if (nameData != nullptr) {
			std::snprintf(lobbies[i].name, sizeof(lobbies[i].name), "%s", nameData);
		} else {
			std::snprintf(lobbies[i].name, sizeof(lobbies[i].name), "%s", Config::UnreadableName.c_str());
		}

		auto password = SteamAPI_ISteamMatchmaking_GetLobbyData(SteamMatchmaking(), lobbyID, Config::LobbyPasswordKey.c_str());
		if (password != nullptr) {
			std::snprintf(lobbies[i].password, sizeof(lobbies[i].password), "%s", password);
		} else {
			std::snprintf(lobbies[i].password, sizeof(lobbies[i].password), "%s", Config::UnreadableName.c_str());
		}

		lobbies[i].memberCount = SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(SteamMatchmaking(), lobbyID);
		lobbies[i].maxPlayers = SteamAPI_ISteamMatchmaking_GetLobbyMemberLimit(SteamMatchmaking(), lobbyID);
        DebugLog("LobbyClient::OnLobbyMatchList %llu: %s, %u, %u, %s\n", 
			lobbies[i].lobbyID, 
			lobbies[i].name, 
			(unsigned)lobbies[i].memberCount, 
			(unsigned)lobbies[i].maxPlayers,
			lobbies[i].password);
	}

	if (this->callbackRequestLobbyList != nullptr) {
		this->callbackRequestLobbyList(0, lobbies, pCallback->m_nLobbiesMatching);
	}
}

void LobbyClient::JoinLobby(
	uint64_steamid lobbyID,
	std::string password,
	CallbackJoinLobby callback
) {

	auto lobbyName = SteamAPI_ISteamMatchmaking_GetLobbyData(SteamMatchmaking(), lobbyID, Config::LobbyNameKey.c_str());
	std::string localLobbyPassword = std::string(lobbyName) + ":" + password;

	this->password = Utils::SHA512(localLobbyPassword);
	this->callbackJoinLobby = callback;

	auto lobbyPassword = SteamAPI_ISteamMatchmaking_GetLobbyData(SteamMatchmaking(), lobbyID, Config::LobbyPasswordKey.c_str());
	if (this->password.compare(lobbyPassword) == 0) {
		SteamAPI_ISteamMatchmaking_JoinLobby(SteamMatchmaking(), lobbyID);
	}
	else {
		DebugLog("LobbyClient::JoinLobby Incorrect password\n");
	}
}

void LobbyClient::OnLobbyEnter(LobbyEnter_t* pCallback) {
	DebugLog("LobbyClient::OnLobbyEnter\n");

	if (pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
		if (this->callbackJoinLobby != NULL) {
			DebugLog("LobbyClient::OnLobbyEnter error\n");
			this->callbackJoinLobby(pCallback->m_EChatRoomEnterResponse, nullptr);
		}
		return;
	}

	SteamAPI_ISteamMatchmaking_SetLobbyMemberData(
		SteamMatchmaking(), 
		pCallback->m_ulSteamIDLobby, 
		Config::LobbyUserNameKey.c_str(), 
		SteamAPI_ISteamFriends_GetPersonaName(SteamFriends()));

	if (this->callbackJoinLobby != NULL) {
		this->callbackJoinLobby(pCallback->m_EChatRoomEnterResponse, new LobbyClientController(pCallback->m_ulSteamIDLobby));
	}
}