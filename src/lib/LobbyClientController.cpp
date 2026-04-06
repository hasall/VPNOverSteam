#include "lib/LobbyClientController.h"

#include "lib/Constants.h"
#include "lib/DebugLog.h"
#include "lib/IpPool.h"
#include "lib/Utils.h"
#include "lib/Config.h"

LobbyClientController::LobbyClientController(CSteamID lobbyID) :
	lobbyID(lobbyID),
	m_CallbackLobbyDataUpdate(this, &LobbyClientController::OnLobbyDataUpdate),
	m_CallbackLobbyChatUpdate(this, &LobbyClientController::OnLobbyChatUpdate),
	m_CallbackLobbyChatMessage(this, &LobbyClientController::OnLobbyChatMessage) {}

LobbyClientController::~LobbyClientController() {
	this->LeaveLobby();
}

void LobbyClientController::SendChatMsg(const char* message) {
	DebugLog("LobbyClientController::SendChatMsg: %s\n", message);
	SteamMatchmaking()->SendLobbyChatMsg(this->lobbyID, message, strlen(message) + 1);
}

void LobbyClientController::LeaveLobby() {
	DebugLog("LobbyClientController::LeaveLobby\n");
	SteamMatchmaking()->LeaveLobby(this->lobbyID);
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
	SteamMatchmaking()->SetLobbyMemberData(this->lobbyID, Config::LobbyUserIpKey.c_str(), Utils::ToString(ip).c_str());
}

void LobbyClientController::GetLobbyMembers() {
	auto memberCount = SteamMatchmaking()->GetNumLobbyMembers(this->lobbyID);
	for (int i = 0; i < memberCount; i++) {
		CSteamID userId = SteamMatchmaking()->GetLobbyMemberByIndex(this->lobbyID, i);
		if (userId == SteamUser()->GetSteamID()) {
			continue; // Skip self
		}
		auto memberName = SteamMatchmaking()->GetLobbyMemberData(this->lobbyID, userId, Config::LobbyUserNameKey.c_str());
        auto memberIp = SteamMatchmaking()->GetLobbyMemberData(this->lobbyID, userId, Config::LobbyUserIpKey.c_str());
		DebugLog("LobbyClientController::GetLobbyMembers: userId: %llu, member name: %s, ip: %s\n", userId.ConvertToUint64(), memberName, memberIp);
        if (this->enterUser != nullptr && memberIp != nullptr) {
			if (memberName == nullptr) memberName = Config::UnreadableName.c_str();
			try {
				this->enterUser(userId.ConvertToUint64(), memberName, Utils::FromString(memberIp));
			}
			catch (const std::exception& ex) {
				DebugLog("LobbyClientController::GetLobbyMembers: exception parsing ip for user %llu: %s\n", userId.ConvertToUint64(), ex.what());
			}
			catch (...) {
				DebugLog("LobbyClientController::GetLobbyMembers: unknown exception for user %llu\n", userId.ConvertToUint64());
			}
		}
	}
}

void LobbyClientController::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback) {
	DebugLog("LobbyClientController::OnLobbyDataUpdate: lobby: %llu, userId: %llu, success: %lu\n", pCallback->m_ulSteamIDLobby, pCallback->m_ulSteamIDMember, (uint32)pCallback->m_bSuccess);
	if (!pCallback->m_bSuccess) {
		return;
	}

    if (pCallback->m_ulSteamIDLobby != pCallback->m_ulSteamIDMember &&
		pCallback->m_ulSteamIDMember != SteamUser()->GetSteamID().ConvertToUint64()) {

		// user data update
		auto memberName = SteamMatchmaking()->GetLobbyMemberData(pCallback->m_ulSteamIDLobby, pCallback->m_ulSteamIDMember, Config::LobbyUserNameKey.c_str());
		auto memberIp = SteamMatchmaking()->GetLobbyMemberData(pCallback->m_ulSteamIDLobby, pCallback->m_ulSteamIDMember, Config::LobbyUserIpKey.c_str());
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
	DebugLog("LobbyClientController::OnLobbyChatUpdate Lobby chat updated: lobby: %llu, userChanged: %llu, makingChange: %llu, stateChange: %lu\n",
		pCallback->m_ulSteamIDLobby,
		pCallback->m_ulSteamIDUserChanged,
		pCallback->m_ulSteamIDMakingChange,
		pCallback->m_rgfChatMemberStateChange);

	if (pCallback->m_ulSteamIDUserChanged == SteamUser()->GetSteamID().ConvertToUint64()) return;

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
			DebugLog("LobbyClientController::OnLobbyChatUpdate Unknown chat member state change: %lu\n", pCallback->m_rgfChatMemberStateChange);
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
	CSteamID steamIDUser = CSteamID(pCallback->m_ulSteamIDUser);
	if (steamIDUser == SteamUser()->GetSteamID()) {
		return; // Ignore own messages
	}
	char message[1024];
	int messageSize = SteamMatchmaking()->GetLobbyChatEntry(pCallback->m_ulSteamIDLobby, pCallback->m_iChatID, &steamIDUser, message, sizeof(message), nullptr);
	if (messageSize <= 0) {
		return; // No message or error
	}
	message[messageSize] = '\0'; // Null-terminate the string
	std::string username = SteamFriends()->GetFriendPersonaName(steamIDUser);
	DebugLog("LobbyClientController::OnLobbyChatMessage: Received chat message from %s: %s\n", username.c_str(), message);
}
