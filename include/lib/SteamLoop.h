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
private:
	bool running;
	std::thread runThread;

	void RunLoop();
};

#endif // STEAM_LOOP_H
