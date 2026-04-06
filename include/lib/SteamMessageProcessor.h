#ifndef STEAM_MESSAGE_PROCESSOR_H
#define STEAM_MESSAGE_PROCESSOR_H

#include <functional>
#include <thread>
#include <string>
#include <steam/steam_api.h>

#define MessageChannel 0
#define SystemMessageChannel 1
#define CallbackReceiveData std::function<void(CSteamID userId, const char* message, size_t size)>

// Prevent collision with Windows API SendMessage macro (expands to SendMessageW/A)
#ifdef SendMessage
#undef SendMessage
#endif

class SteamMessageProcessor {
public:
	SteamMessageProcessor(
		CallbackReceiveData messageReceiver,
		CallbackReceiveData systemMessageReceiver
	);
	~SteamMessageProcessor();

	void Start();
	void Stop();

	void SendMessage(SteamNetworkingIdentity userId, const char* data, size_t size);
	void SendSystemMessage(SteamNetworkingIdentity userId, const char* data, size_t size);

private:
	CallbackReceiveData messageReceiver;
	CallbackReceiveData systemMessageReceiver;

	bool running = false;
	std::thread runMessageThread = {};
	std::thread runSystemMessageThread = {};

	void SendData(SteamNetworkingIdentity userId, const char* data, size_t size, int channel);
	void ReceiveDataLoop(int channel, CallbackReceiveData callback);

	STEAM_CALLBACK(SteamMessageProcessor, OnConnectionRequested, SteamNetworkingMessagesSessionRequest_t);
	STEAM_CALLBACK(SteamMessageProcessor, OnConnectionFailed, SteamNetworkingMessagesSessionFailed_t);
};

#endif // STEAM_MESSAGE_PROCESSOR_H
