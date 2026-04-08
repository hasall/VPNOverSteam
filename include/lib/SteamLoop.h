#ifndef STEAM_LOOP_H
#define STEAM_LOOP_H

#include <iostream>
#include <thread>
#include <vector>

class SteamLoop {
public:
	SteamLoop();
	~SteamLoop();

	bool Start();
	void Stop();
	
	bool StartServer();
	bool StopServer();

private:
	bool running;
	std::thread runThread;

	void RunLoop();
	void RunServerLoop();
};

#endif // STEAM_LOOP_H
