#include <mutex>
#include <condition_variable>

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
    
struct CLARGS {
	char appType; // 's' for server, 'c' for client
	std::string lobbyName;
	std::string lobbyPassword;
};
    
int client_start(CLARGS args) {
    std::mutex mtx;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock(mtx);
	bool mtxReady = false;


	std::cout << "Initializing..." << std::endl;
	SteamLoop steamLoop;
	auto steamLoopResult = steamLoop.Start();
	if (!steamLoopResult) {
		std::cerr << "Failed to start SteamLoop\n";
		return 1;
	}

	LobbyClient lobbyClient;
	LobbyClientController* lobbyControllerClient = nullptr;
	LobbyList findLobby = { 0 };

	Client client(
		[&lobbyControllerClient](uint32_t ip) {
			lobbyControllerClient->SetIp(ip);
		}
	#ifdef _WIN32
		, Config::AdapterGuid
	#endif
	);

	std::cout << "Searching rooms..." << std::endl;
	auto lobbyName = args.lobbyName;
	lobbyClient.RequestLobbyList([&findLobby, &lobbyName, &cv, &mtxReady](uint32_t result, LobbyList* lobby, int lobbyListSize) {
		if (result != 0) {
			std::cerr << "RequestLobbyList no lobby found\n";
			return;
		}

		for (int i = 0; i < lobbyListSize; i++) {
			std::cout << i << ": " << lobby[i].name << " (" << lobby[i].memberCount << "/" << lobby[i].maxPlayers << ")\n";
			if (lobby[i].name == lobbyName) {
				findLobby = lobby[i];
				break;
			}
		}

        delete [] lobby;

        mtxReady = true;
		cv.notify_one();
	});

    mtxReady = false;
	if (cv.wait_for(lock, std::chrono::seconds(10), [&mtxReady] { return mtxReady; })) {
		std::cerr << "Failed to get lobby list\n";
		return 1;
	}

    if (findLobby.lobbyID == 0) {
        std::cerr << "Lobby not found\n";
        return 1;
    }

    auto password = args.lobbyPassword;
    lobbyClient.JoinLobby(
        findLobby.lobbyID, 
        password, 
        [&lobbyControllerClient, &client, &password, &cv, &mtxReady](uint32_t result, LobbyClientController* lobbyController) {
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

            mtxReady = true;
            cv.notify_one();
    });

    mtxReady = false;
    if (cv.wait_for(lock, std::chrono::seconds(10), [&mtxReady] { return mtxReady; })) {
		std::cerr << "Failed to join lobby\n";
		return 1;
	}

    std::cout << "Press any key for exit...\n";
    std::cin.get();

    std::cout << "Quitting..." << std::endl;
    lobbyControllerClient->LeaveLobby();
    std::this_thread::sleep_for(std::chrono::seconds(10)); // wait for steam messages processing
	steamLoop.Stop();
	client.Stop();

    return 0;
}

int server_start(CLARGS args) {
    std::mutex mtx;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock(mtx);
	bool mtxReady = false;


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

    std::cout << "Creating room..." << std::endl;
    auto lobbyPassword = args.lobbyPassword;
    lobbyServer.CreateLobby(args.lobbyName, args.lobbyPassword, [&server, &lobbyControllerServer, &lobbyPassword, &cv, &mtxReady](uint32_t result, LobbyServerController* lobbyController) {
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

        mtxReady = true;
        cv.notify_one();
    });

    mtxReady = false;
    if (cv.wait_for(lock, std::chrono::seconds(10), [&mtxReady] { return mtxReady; })) {
		std::cerr << "Failed to join lobby\n";
		return 1;
	}

    std::cout << "Press any key for exit...\n";
    std::cin.get();

    std::cout << "Quitting..." << std::endl;
    lobbyControllerServer->LeaveLobby();
    std::this_thread::sleep_for(std::chrono::seconds(10)); // wait for steam messages processing
	steamLoop.Stop();
	server.Stop();

    return 0;
}

CLARGS parse_args(int argc, char* argv[]) {
    CLARGS args = { 0, "", "" };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--server" || arg == "-s") {
            args.appType = 's';
        } 
        if (arg == "--client" || arg == "-c") {
            args.appType = 'c';
        } 
        if ((arg == "--lobby-name" || arg == "-ln") && i + 1 < argc) {
            args.lobbyName = argv[++i];
        } 
        if ((arg == "--lobby-password" || arg == "-lp") && i + 1 < argc) {
            args.lobbyPassword = argv[++i];
        }
    }

    if (args.appType == 0) {
        std::cerr << "Mast specify --server or --client\n";
        exit(1);
    }
    if (args.lobbyName.empty()) {
        std::cerr << "Mast specify lobby name\n";
        exit(1);
    }
    if (args.lobbyPassword.empty()) {
        std::cerr << "Mast specify lobby password\n";
        exit(1);
    }

    return args;
}

int AutomaticMain(int argc, char* argv[])
{
	CLARGS args = parse_args(argc, argv);
		
	try {
		Config::Init();
	
		if (args.appType == 'c') {
			client_start(args);
		}
		if (args.appType == 's') {
			server_start(args);
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