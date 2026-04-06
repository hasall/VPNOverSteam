#ifndef LOBBY_CLIENT_CONTROLLER_H
#define LOBBY_CLIENT_CONTROLLER_H

#include <functional>
#include <steam/steam_api.h>

#include <string>

#define CallbackClientEnteredUser std::function<void(uint64 userId, const char* userName, uint32_t ip)>
#define CallbackClientLeftUser std::function<void(uint64 userId)>

class LobbyClientController {
public:
	LobbyClientController(CSteamID lobbyID);
	~LobbyClientController();

	void SendChatMsg(const char* message);
	void LeaveLobby();

	void SetNewUserCallbacks(
		CallbackClientEnteredUser enterUser,
		CallbackClientLeftUser leftUser);

	void SetIp(uint32_t ip);

private:
	CSteamID lobbyID = {};
	CallbackClientEnteredUser enterUser = nullptr;
	CallbackClientLeftUser leftUser = nullptr;

	void GetLobbyMembers();

	STEAM_CALLBACK(LobbyClientController, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate);
	STEAM_CALLBACK(LobbyClientController, OnLobbyChatUpdate, LobbyChatUpdate_t, m_CallbackLobbyChatUpdate);
	STEAM_CALLBACK(LobbyClientController, OnLobbyChatMessage, LobbyChatMsg_t, m_CallbackLobbyChatMessage);
};

#endif // LOBBY_CLIENT_CONTROLLER_H
