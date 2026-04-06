#include "lib/LobbyServerController.h"

#include <inttypes.h>

#include "lib/Constants.h"
#include "lib/DebugLog.h"
#include "lib/Utils.h"
#include "lib/Config.h"

LobbyServerController::LobbyServerController(uint64_steamid lobbyID) :
	m_CallbackLobbyDataUpdate(this, &LobbyServerController::OnLobbyDataUpdate),
	m_CallbackLobbyChatUpdate(this, &LobbyServerController::OnLobbyChatUpdate),
	m_CallbackLobbyChatMessage(this, &LobbyServerController::OnLobbyChatMessage)
{
	this->lobbyID = lobbyID;
}

void LobbyServerController::SendChatMsg(const char* message) {
	DebugLog("LobbyServerController::SendChatMsg: %s\n", message);
	SteamAPI_ISteamMatchmaking_SendLobbyChatMsg(SteamMatchmaking(), this->lobbyID, message, strlen(message) + 1);

}

void LobbyServerController::LeaveLobby() {
	SteamAPI_ISteamMatchmaking_LeaveLobby(SteamMatchmaking(), this->lobbyID);
	this->lobbyID = {};
}

void LobbyServerController::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback) {
	DebugLog(
		"LobbyServerController::OnLobbyDataUpdate: lobby: %llu, userId: %llu, success: %u\n", 
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

        DebugLog("LobbyServerController::OnLobbyDataUpdate username: %s, ip: %s\n", memberName, memberIp);
		if (this->enterUser != nullptr && memberIp != nullptr) {
			if (memberName == nullptr) memberName = Config::UnreadableName.c_str();
			try {
				this->enterUser(pCallback->m_ulSteamIDMember, memberName, Utils::FromString(memberIp));
			}
			catch (const std::exception& ex) {
				DebugLog("LobbyServerController::OnLobbyDataUpdate: exception in enterUser callback: %s\n", ex.what());
			}
			catch (...) {
				DebugLog("LobbyServerController::OnLobbyDataUpdate: unknown exception in enterUser callback\n");
			}
		}
	}
	else {
		// lobby data update
		DebugLog("LobbyServerController::OnLobbyDataUpdate lobby data updated\n");
	}
}

void LobbyServerController::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback) {
	DebugLog("LobbyServerController::OnLobbyChatUpdate Lobby chat updated: lobby: %llu, userChanged: %llu, makingChange: %llu, stateChange: %u\n",
		pCallback->m_ulSteamIDLobby,
		pCallback->m_ulSteamIDUserChanged,
		pCallback->m_ulSteamIDMakingChange,
		pCallback->m_rgfChatMemberStateChange);

	if (pCallback->m_ulSteamIDUserChanged == SteamAPI_ISteamUser_GetSteamID(SteamUser())) return;

	switch (pCallback->m_rgfChatMemberStateChange) {
	case k_EChatMemberStateChangeEntered:
		DebugLog("LobbyServerController::OnLobbyChatUpdate User entered the lobby\n");
		break;
	case k_EChatMemberStateChangeLeft:
		DebugLog("LobbyServerController::OnLobbyChatUpdate User left the lobby\n");
		if (this->leftUser != nullptr) {
			this->leftUser(pCallback->m_ulSteamIDUserChanged);
		}
		break;
	case k_EChatMemberStateChangeDisconnected:
		DebugLog("LobbyServerController::OnLobbyChatUpdate User disconnected from the lobby\n");
		if (this->leftUser != nullptr) {
			this->leftUser(pCallback->m_ulSteamIDUserChanged);
		}
		break;
	case k_EChatMemberStateChangeKicked:
		DebugLog("LobbyServerController::OnLobbyChatUpdate User was kicked from the lobby\n");
		if (this->leftUser != nullptr) {
			this->leftUser(pCallback->m_ulSteamIDUserChanged);
		}
		break;
	case k_EChatMemberStateChangeBanned:
		DebugLog("LobbyServerController::OnLobbyChatUpdate User was banned from the lobby\n");
		if (this->leftUser != nullptr) {
			this->leftUser(pCallback->m_ulSteamIDUserChanged);
		}
		break;
	default:
		DebugLog("LobbyServerController::OnLobbyChatUpdate Unknown chat member state change: %u\n", pCallback->m_rgfChatMemberStateChange);
	}
}

void LobbyServerController::OnLobbyChatMessage(LobbyChatMsg_t* pCallback) {
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
	DebugLog("LobbyServerController::OnLobbyChatMessage: Received chat message from %s: %s\n", username.c_str(), message);
}

void LobbyServerController::KickMember(uint64_steamid userId) {
	DebugLog("LobbyServerController::KickMember\n");
}

void LobbyServerController::BanMember(uint64_steamid userId) {
	DebugLog("LobbyServerController::BanMember\n");
}

void LobbyServerController::SetNewUserCallbacks(
	CallbackServerEnteredUser enterUser,
	CallbackServerLeftUser leftUser
) {
	this->enterUser = enterUser;
	this->leftUser = leftUser;
}