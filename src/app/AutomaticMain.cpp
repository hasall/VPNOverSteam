#include <mutex>
#include <condition_variable>

#include <iostream>
#include <functional>

#include "lib/Constants.h"
#include "lib/SteamLoop.h"

#include "lib/Client.h"
#include "lib/Server.h"

#include "lib/DebugLog.h"
//#include "lib/TUNMock.h"
#include "lib/Config.h"
#include "lib/Utils.h"
    
struct CLARGS {
	char appType; // 's' for server, 'c' for client
	uint64 lobbyId;
	std::string lobbyPassword;
};
    
int client_start(CLARGS args) {
    std::mutex mtx;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock(mtx);
	bool mtxReady = false;


	std::cout << "Initializing..." << std::endl;
	SteamLoop steamLoop;
	if (!steamLoop.StartServer()) {
		std::cerr << "Failed to start SteamLoop\n";
		return 1;
	}

	Client client(
		[&mtxReady, &cv](uint32_t ip) {
            DebugLog("Client::ReceiveNewIpCallback: received ip: %u, %s\n", ip, Utils::ToString(ip).c_str());

            mtxReady = true;
            cv.notify_one();
		}
	#ifdef _WIN32
		, Config::AdapterGuid
	#endif
	);

    client.Start(args.lobbyId, args.lobbyPassword);

    mtxReady = false;
	if (cv.wait_for(lock, std::chrono::seconds(10), [&mtxReady] { return mtxReady; })) {
		std::cerr << "Failed to get lobby list\n";
		return 1;
	}

    std::cout << "Press any key for exit...\n";
    std::cin.get();

    std::cout << "Quitting..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10)); // wait for steam messages processing
	client.Stop();
	steamLoop.StopServer();

    return 0;
}

int server_start(CLARGS args) {
    std::mutex mtx;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock(mtx);
	bool mtxReady = false;


	std::cout << "Initializing..." << std::endl;
	SteamLoop steamLoop;
	if (!steamLoop.StartServer()) {
		DebugLog("Failed to start SteamLoop\n");
		return 1;
	}

	#ifdef _WIN32
	Server server(Config::AdapterGuid);
	#else
	Server server;
	#endif
        
    server.Start(args.lobbyPassword);


    std::cout << "Press any key for exit...\n";
    std::cin.get();

    std::cout << "Quitting..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(10)); // wait for steam messages processing
	server.Stop();
	steamLoop.StopServer();

    return 0;
}

CLARGS parse_args(int argc, char* argv[]) {
    CLARGS args = { 0, 0, "" };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--server" || arg == "-s") {
            args.appType = 's';
        } 
        if (arg == "--client" || arg == "-c") {
            args.appType = 'c';
        } 
        if ((arg == "--lobby-id" || arg == "-li") && i + 1 < argc) {
            args.lobbyId = std::stoull(argv[++i]);
        } 
        if ((arg == "--lobby-password" || arg == "-lp") && i + 1 < argc) {
            args.lobbyPassword = argv[++i];
        }
    }

    if (args.appType == 0) {
        std::cerr << "Mast specify --server or --client\n";
        exit(1);
    }
    if (args.lobbyId == 0 && args.appType == 'c') {
        std::cerr << "Mast specify lobby ID\n";
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