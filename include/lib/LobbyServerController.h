#ifndef LOBBY_SERVER_CONTROLLER_H
#define LOBBY_SERVER_CONTROLLER_H

#include <functional>
#include <steam/steam_api.h>

#include <string>


#define CallbackServerEnteredUser std::function<void(uint64 userId, const char* userName, uint32_t ip)>
#define CallbackServerLeftUser std::function<void(uint64 userId)>

class LobbyServerController {
public:
	LobbyServerController(CSteamID lobbyID);

	~LobbyServerController() {
		this->LeaveLobby();
	}
	void SendChatMsg(const char* message);
	void LeaveLobby();

	void KickMember(CSteamID userId);
	void BanMember(CSteamID userId);

	void SetNewUserCallbacks(
		CallbackServerEnteredUser enterUser,
		CallbackServerLeftUser leftUser);

private:
	CSteamID lobbyID = {};
	CallbackServerEnteredUser enterUser = nullptr;
	CallbackServerLeftUser leftUser = nullptr;


	STEAM_CALLBACK(LobbyServerController, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate);
	STEAM_CALLBACK(LobbyServerController, OnLobbyChatUpdate, LobbyChatUpdate_t, m_CallbackLobbyChatUpdate);
	STEAM_CALLBACK(LobbyServerController, OnLobbyChatMessage, LobbyChatMsg_t, m_CallbackLobbyChatMessage);
};

#endif // LOBBY_SERVER_CONTROLLER_H
