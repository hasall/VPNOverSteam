// VPNOverSteamConsole.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <functional>

#include "lib/Constants.h"
#include "lib/SteamLoop.h"

#include "lib/Client.h"
#include "lib/LobbyClient.h"
#include "lib/LobbyClientController.h"

#include "lib/Server.h"
#include "lib/LobbyServer.h"
#include "lib/LobbyServerController.h"

#include "lib/DebugLog.h"
//#include "lib/TUNMock.h"
#include "lib/Config.h"
#include "lib/Utils.h"

int client_start() {
	std::string input;

	std::cout << "Initializing..." << std::endl;
	SteamLoop steamLoop;
	auto steamLoopResult = steamLoop.Start();
	if (!steamLoopResult) {
		DebugLog("Failed to start SteamLoop\n");
		return 1;
	}

	LobbyClient lobbyClient;
	LobbyClientController* lobbyControllerClient = nullptr;
	LobbyList* lobbies = nullptr;
	size_t lobbiesSize = 0;

	Client client(
		[&lobbyControllerClient](uint32_t ip) {
			lobbyControllerClient->SetIp(ip);
		}
	#ifdef _WIN32
		, Config::AdapterGuid
	#endif
	);

	std::cout << "q - Quit" << std::endl;
	std::cout << "s - Search rooms" << std::endl;
	std::cout << "j - Join to room" << std::endl;
	std::cout << "w - Write message" << std::endl;
	std::cout << "l - Leave room" << std::endl;

	while (true) {
		std::cout << "Enter commands: \n";
		std::getline(std::cin, input);

		switch (input[0]) {

		case 'q': {
			std::cout << "Quitting..." << std::endl;
			steamLoop.Stop();
			client.Stop();
			return 0;
		}

		case 's': {
			std::cout << "Searching rooms..." << std::endl;
			lobbyClient.RequestLobbyList([&lobbies, &lobbiesSize](uint32_t result, LobbyList* lobby, int lobbyListSize) {
				if (result != 0) {
					DebugLog("RequestLobbyList no lobby found\n");
					return;
				}
				lobbies = lobby;
				lobbiesSize = lobbyListSize;
				});
			break;
		}

		case 'j': {
			std::cout << "Joining to room..." << std::endl;

			std::cout << "Enter room number: ";
			std::string n_str;
			std::getline(std::cin, n_str);
			int n = std::stoi(n_str);

			if (n < 0 || n >= lobbiesSize) {
				DebugLog("Join error\n");
				break;
			}

			std::cout << std::endl;
			std::cout << "Enter room password: ";
			std::string password;
			std::getline(std::cin, password);

			lobbyClient.JoinLobby(lobbies[n].lobbyID, password, [&lobbyControllerClient, &client, &password](uint32_t result, LobbyClientController* lobbyController) {
				if (result != k_EChatRoomEnterResponseSuccess) {
					DebugLog("JoinLobby error\n");
					return;
				}
				client.Start(password);

				lobbyControllerClient = lobbyController;
				lobbyControllerClient->SetNewUserCallbacks(
					std::bind(&Client::JoinMember, &client, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
					std::bind(&Client::LeftMember, &client, std::placeholders::_1)
				);
				});
			break;
		}

		case 'w': {
			std::cout << "Write message..." << std::endl;
			std::cout << "Message: ";
			std::string message;
			std::getline(std::cin, message);

			lobbyControllerClient->SendChatMsg(message.c_str());
			break;
		}

		case 'l': {
			std::cout << "Leave room..." << std::endl;
			lobbyControllerClient->LeaveLobby();
			break;
		}

		default:
			break;
		}
	}
}

int server_start() {
	std::string input;

	std::cout << "Initializing..." << std::endl;
	SteamLoop steamLoop;
	auto steamLoopResult = steamLoop.StartServer();
	if (!steamLoopResult) {
		DebugLog("Failed to start SteamLoop\n");
		return 1;
	}

	LobbyServer lobbyServer;
	LobbyServerController* lobbyControllerServer = nullptr;

	#ifdef _WIN32
	Server server(Config::AdapterGuid);
	#else
	Server server;
	#endif

	std::cout << "q - Quit" << std::endl;
	std::cout << "c - Create room" << std::endl;
	std::cout << "k - Kick from room" << std::endl;
	std::cout << "w - Write message" << std::endl;
	std::cout << "l - Leave room" << std::endl;

	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	if (!std::cin) {
		DebugLog("Failed to clear cin\n");
		return 1;
	}
	while (true) {
		std::cout << "Enter commands: \n";
		std::cout << "state: fail=" << std::cin.fail()
          << " eof=" << std::cin.eof()
          << " bad=" << std::cin.bad() << std::endl;
		std::getline(std::cin, input);
		std::cout << "state: fail=" << std::cin.fail()
          << " eof=" << std::cin.eof()
          << " bad=" << std::cin.bad() << std::endl;

		switch (input[0]) {

		case 'q': {
			std::cout << "Quitting..." << std::endl;
			steamLoop.Stop();
			server.Stop();
			return 0;
		}

		case 'c': {
			std::cout << "Creating room..." << std::endl;
			std::cout << "Enter lobby name: ";
			std::string lobbyName;
			std::getline(std::cin, lobbyName);

			std::cout << "Enter lobby password: ";
			std::string lobbyPassword;
			std::getline(std::cin, lobbyPassword);

			lobbyServer.CreateLobby(lobbyName, lobbyPassword, [&server, &lobbyControllerServer, &lobbyPassword](uint32_t result, LobbyServerController* lobbyController) {
				if (result != k_EChatRoomEnterResponseSuccess) {
					DebugLog("JoinLobby error\n");
					return;
				}

				server.Start(lobbyPassword);

				lobbyControllerServer = lobbyController;
				lobbyControllerServer->SetNewUserCallbacks(
					std::bind(&Server::JoinMember, &server, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
					std::bind(&Server::LeftMember, &server, std::placeholders::_1)
				);
				});
			break;
		}

		case 'k': {
			std::cout << "Kicking from room..." << std::endl;
			std::cout << "Member Id: ";

			uint64 userId;
			std::getline(std::cin, input);
			userId = std::stoull(input);

			CSteamID id(userId);

			lobbyControllerServer->KickMember(userId);
			break;
		}
		case 'w': {
			std::cout << "Write message..." << std::endl;
			std::cout << "Message: ";
			std::string message;
			std::getline(std::cin, message);

			lobbyControllerServer->SendChatMsg(message.c_str());
			break;
		}

		case 'l': {
			std::cout << "Leave room..." << std::endl;
			lobbyControllerServer->LeaveLobby();
			break;
		}
		default:
			break;

		}
	}
}


int ManualMain()
{
	try {
		Config::Init();
		
		std::string type;
		std::cout << "Enter type ((c)lient | (s)erver | (q)uit): ";
		std::getline(std::cin, type);

		if (type[0] == 'c') {
			client_start();
		}
		if (type[0] == 's') {
			server_start();
		}
	}
	catch (std::exception &e) {
		DebugLog("Main thread exception: %s\n", e.what());
	}
	catch (...) {
		DebugLog("Main thread unknown exception\n");
	}

	std::cout << "Press Enter to continue..." << std::endl;
    std::cin.get(); // Waits for a single Enter key press

	return 0;
}
