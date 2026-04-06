#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <functional>
#include <steam/steam_api_flat.h>

#include "LobbyClientController.h"
#include "Constants.h"

#define CallbackJoinLobby std::function<void(uint32_t result, LobbyClientController* lobbyController)>
#define CallbackRequestLobbyList std::function<void(uint32_t result, LobbyList* lobby, int lobbyListSize)>

struct LobbyList {
    uint64_steamid lobbyID = {};
    int memberCount = 0;
    int maxPlayers = 0;
    char name[96] = "";
    char password[SHA512_SIZE + 1];
};

class LobbyClient {
public:
    LobbyClient();
    ~LobbyClient();

    void RequestLobbyList(CallbackRequestLobbyList callback);

    void JoinLobby(
        uint64_steamid lobbyID,
        std::string passwprd,
        CallbackJoinLobby callback
    );

private:
    std::string password;

    CallbackJoinLobby callbackJoinLobby = nullptr;
    CallbackRequestLobbyList callbackRequestLobbyList = nullptr;

    STEAM_CALLBACK(LobbyClient, OnLobbyMatchList, LobbyMatchList_t, m_CallbackLobbyMatchList);
    STEAM_CALLBACK(LobbyClient, OnLobbyEnter, LobbyEnter_t, m_CallbackLobbyEnter);
};

#endif // LOBBY_CLIENT_H
