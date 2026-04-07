#include "lib/LobbyClientController.h"

#include "lib/Constants.h"
#include "lib/DebugLog.h"
#include "lib/IpPool.h"
#include "lib/Utils.h"
#include "lib/Config.h"

LobbyClientController::LobbyClientController(uint64_steamid lobbyID) :
	lobbyID(lobbyID),
	m_CallbackLobbyDataUpdate(this, &LobbyClientController::OnLobbyDataUpdate),
	m_CallbackLobbyChatUpdate(this, &LobbyClientController::OnLobbyChatUpdate),
	m_CallbackLobbyChatMessage(this, &LobbyClientController::OnLobbyChatMessage) {}

LobbyClientController::~LobbyClientController() {
	this->LeaveLobby();
}

void LobbyClientController::SendChatMsg(const char* message) {
	DebugLog("LobbyClientController::SendChatMsg: %s\n", message);
	SteamAPI_ISteamMatchmaking_SendLobbyChatMsg(SteamMatchmaking(), this->lobbyID, message, strlen(message) + 1);
}

void LobbyClientController::LeaveLobby() {
	DebugLog("LobbyClientController::LeaveLobby\n");
	SteamAPI_ISteamMatchmaking_LeaveLobby(SteamMatchmaking(), this->lobbyID);
	this->lobbyID = {};
}

void LobbyClientController::SetNewUserCallbacks(
	CallbackClientEnteredUser enterUser,
	CallbackClientLeftUser leftUser
) {
	this->enterUser = enterUser;
	this->leftUser = leftUser;

	this->GetLobbyMembers();
}

void LobbyClientController::SetIp(uint32_t ip) {
	SteamAPI_ISteamMatchmaking_SetLobbyMemberData(SteamMatchmaking(), this->lobbyID, Config::LobbyUserIpKey.c_str(), Utils::ToString(ip).c_str());
}

void LobbyClientController::GetLobbyMembers() {
	auto memberCount = SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(SteamMatchmaking(), this->lobbyID);
	for (int i = 0; i < memberCount; i++) {
		uint64_steamid userId = SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(SteamMatchmaking(), this->lobbyID, i);
		if (userId == SteamAPI_ISteamUser_GetSteamID(SteamUser())) {
			continue; // Skip self
		}
		auto memberName = SteamAPI_ISteamMatchmaking_GetLobbyMemberData(SteamMatchmaking(), this->lobbyID, userId, Config::LobbyUserNameKey.c_str());
        auto memberIp = SteamAPI_ISteamMatchmaking_GetLobbyMemberData(SteamMatchmaking(), this->lobbyID, userId, Config::LobbyUserIpKey.c_str());
		DebugLog("LobbyClientController::GetLobbyMembers: userId: %llu, member name: %s, ip: %s\n", userId, memberName, memberIp);
        if (this->enterUser != nullptr && memberIp != nullptr) {
			if (memberName == nullptr) memberName = Config::UnreadableName.c_str();
			try {
				this->enterUser(userId, memberName, Utils::FromString(memberIp));
			}
			catch (const std::exception& ex) {
				DebugLog("LobbyClientController::GetLobbyMembers: exception parsing ip for user %llu: %s\n", userId, ex.what());
			}
			catch (...) {
				DebugLog("LobbyClientController::GetLobbyMembers: unknown exception for user %llu\n", userId);
			}
		}
	}
}

void LobbyClientController::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback) {
	DebugLog(
		"LobbyClientController::OnLobbyDataUpdate: lobby: %llu, userId: %llu, success: %u\n",
		pCallback->m_ulSteamIDLobby,
		pCallback->m_ulSteamIDMember,
		(uint32)pCallback->m_bSuccess);
	if (!pCallback->m_bSuccess) {
		return;
	}

    if (pCallback->m_ulSteamIDLobby != pCallback->m_ulSteamIDMember &&
		pCallback->m_ulSteamIDMember != SteamAPI_ISteamUser_GetSteamID(SteamUser())) {

		// user data update
		auto memberName = SteamAPI_ISteamMatchmaking_GetLobbyMemberData(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, pCallback->m_ulSteamIDMember, Config::LobbyUserNameKey.c_str());
		auto memberIp = SteamAPI_ISteamMatchmaking_GetLobbyMemberData(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, pCallback->m_ulSteamIDMember, Config::LobbyUserIpKey.c_str());
		DebugLog("LobbyClientController::OnLobbyDataUpdate username: %s, ip: %s\n", memberName, memberIp);
		if (this->enterUser != nullptr && memberIp != nullptr) {
			if (memberName == nullptr) memberName = Config::UnreadableName.c_str();
			try {
				this->enterUser(pCallback->m_ulSteamIDMember, memberName, Utils::FromString(memberIp));
			}
			catch (const std::exception& ex) {
				DebugLog("LobbyClientController::OnLobbyDataUpdate: exception parsing ip: %s\n", ex.what());
			}
			catch (...) {
				DebugLog("LobbyClientController::OnLobbyDataUpdate: unknown exception\n");
			}
		}
	}
	else {
		// lobby data update
		DebugLog("LobbyClientController::OnLobbyDataUpdate lobby data updated\n");
	}
}

void LobbyClientController::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback) {
	DebugLog("LobbyClientController::OnLobbyChatUpdate Lobby chat updated: lobby: %llu, userChanged: %llu, makingChange: %llu, stateChange: %u\n",
		pCallback->m_ulSteamIDLobby,
		pCallback->m_ulSteamIDUserChanged,
		pCallback->m_ulSteamIDMakingChange,
		pCallback->m_rgfChatMemberStateChange);

	if (pCallback->m_ulSteamIDUserChanged == SteamAPI_ISteamUser_GetSteamID(SteamUser())) return;

	try {
		switch (pCallback->m_rgfChatMemberStateChange) {
		case k_EChatMemberStateChangeEntered:
			DebugLog("LobbyClientController::OnLobbyChatUpdate User entered the lobby\n");
			break;
		case k_EChatMemberStateChangeLeft:
			DebugLog("LobbyClientController::OnLobbyChatUpdate User left the lobby\n");
			if (this->leftUser != nullptr) {
				this->leftUser(pCallback->m_ulSteamIDUserChanged);
			}
			break;
		case k_EChatMemberStateChangeDisconnected:
			DebugLog("LobbyClientController::OnLobbyChatUpdate User disconnected from the lobby\n");
			if (this->leftUser != nullptr) {
				this->leftUser(pCallback->m_ulSteamIDUserChanged);
			}
			break;
		case k_EChatMemberStateChangeKicked:
			DebugLog("LobbyClientController::OnLobbyChatUpdate User was kicked from the lobby\n");
			if (this->leftUser != nullptr) {
				this->leftUser(pCallback->m_ulSteamIDUserChanged);
			}
			break;
		case k_EChatMemberStateChangeBanned:
			DebugLog("LobbyClientController::OnLobbyChatUpdate User was banned from the lobby\n");
			if (this->leftUser != nullptr) {
				this->leftUser(pCallback->m_ulSteamIDUserChanged);
			}
			break;
		default:
			DebugLog("LobbyClientController::OnLobbyChatUpdate Unknown chat member state change: %u\n", pCallback->m_rgfChatMemberStateChange);
		}
	}
	catch (const std::exception& ex) {
		DebugLog("LobbyClientController::OnLobbyDataUpdate: exception parsing ip: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("LobbyClientController::OnLobbyDataUpdate: unknown exception\n");
	}
}

void LobbyClientController::OnLobbyChatMessage(LobbyChatMsg_t* pCallback) {
	if (pCallback->m_eChatEntryType != k_EChatEntryTypeChatMsg) {
		return;
	}
	if (pCallback->m_ulSteamIDUser == SteamAPI_ISteamUser_GetSteamID(SteamUser())) {
		return; // Ignore own messages
	}
	auto steamIDUser = CSteamID(pCallback->m_ulSteamIDUser);
	char message[1024];
	int messageSize = SteamAPI_ISteamMatchmaking_GetLobbyChatEntry(SteamMatchmaking(), pCallback->m_ulSteamIDLobby, pCallback->m_iChatID, &steamIDUser, message, sizeof(message), nullptr);
	if (messageSize <= 0) {
		return; // No message or error
	}
	message[messageSize] = '\0'; // Null-terminate the string
	std::string username = SteamAPI_ISteamFriends_GetFriendPersonaName(SteamFriends(), pCallback->m_ulSteamIDUser);
	DebugLog("LobbyClientController::OnLobbyChatMessage: Received chat message from %s: %s\n", username.c_str(), message);
}
