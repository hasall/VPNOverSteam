#ifndef LOBBY_SERVER_H
#define LOBBY_SERVER_H

#include <functional>
#include <string>
#include <steam/steam_api_flat.h>

#include "LobbyServerController.h"

#define CallbackCreateLobby std::function<void(uint32_t result, LobbyServerController* lobbyController)>

class LobbyServer {
public:
	LobbyServer();

	void CreateLobby(
		std::string name,
		std::string password,
		CallbackCreateLobby callback
	);

private:
	CallbackCreateLobby callbackCreateLobby = nullptr;

	std::string lobbyName = {};
	std::string password = {};
	
	STEAM_CALLBACK(LobbyServer, OnLobbyCreated, LobbyCreated_t, m_CallbackLobbyCreated);
	STEAM_CALLBACK(LobbyServer, OnLobbyEnter, LobbyEnter_t, m_CallbackLobbyEnter);
};

#endif // LOBBY_SERVER_H
