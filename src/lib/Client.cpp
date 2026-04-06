#include <functional>
#include <stdexcept>

#include "lib/Client.h"
#include "lib/DebugLog.h"
#include "lib/Constants.h"
#include "lib/Messages.h"
#include "lib/Utils.h"
#include "lib/Config.h"

#ifdef _WIN32
Client::Client(ReceiveNewIpCallback callback, GUID guid) :
#else
Client::Client(ReceiveNewIpCallback callback) :
#endif
	callback(callback),
	password(),
	tunMessageProcessor(
		std::bind(&Client::TUNDataReceiver, this, std::placeholders::_1, std::placeholders::_2)
		#ifdef _WIN32
		,
		guid
		#endif
	),
	steamMessageProcessor(
		std::bind(&Client::SteamMessageReceiver, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&Client::SteamSystemMessageReceiver, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	),
	mtx(),
	cv(),
	lock(mtx),
	mtx_ready(false)
{

}

Client::~Client() {
	this->Stop();
	lock.release();
}

void Client::JoinMember(uint64 userId, const char* userName, uint32_t ip) {
    DebugLog("Client::JoinMember id: %llu, name: %s, ip: %s\n", userId, userName, Utils::ToString(ip).c_str());
	
	auto user = SteamNetworkingIdentity();
	user.SetSteamID(CSteamID(userId));
	auto res = this->ipToClient.insert({ ip, user });
	if (!res.second) {
		DebugLog("Client::JoinMember: ip %s already present in map, replacing entry\n", Utils::ToString(ip).c_str());
		return;
	}

	if (Utils::ToString(ip).compare(Config::ServerIp) == 0) {
		this->SendPasswordMassage(user);
	}
}

void Client::LeftMember(uint64 userId) {
    DebugLog("Client::LeftMember id: %llu\n", userId);
	auto it = std::find_if(this->ipToClient.begin(), this->ipToClient.end(),
		[&userId](const std::pair<uint32_t, SteamNetworkingIdentity>& pair) {
			return pair.second.GetSteamID() == userId;
		});
	if (it == this->ipToClient.end()) {
		DebugLog("Client::LeftMember: userId %llu not found in map\n", userId);
	} else {
		SteamAPI_ISteamNetworkingMessages_CloseSessionWithUser(SteamNetworkingMessages(), it->second);
		this->ipToClient.erase(it);
	}
}

void Client::Start(std::string password) {
	this->password = Utils::SHA512(password);
	this->steamMessageProcessor.Start();
}

void Client::Stop() {
	this->steamMessageProcessor.Stop();
	this->tunMessageProcessor.Stop();
}

void Client::TUNDataReceiver(const char* message, size_t size) {
    // send to steam
	if (message == nullptr) {
		DebugLog("Client::TUNDataReceiver: null message received\n");
		return;
	}
	if (size < sizeof(ip_header)) {
		DebugLog("Client::TUNDataReceiver: message size too small (%zu)\n", size);
		return;
	}
	auto ipHeader = reinterpret_cast<const ip_header*>(message);
	auto it = this->ipToClient.find(ipHeader->dest_ip);
	if (it == this->ipToClient.end()) {
		DebugLog("Client::TUNDataReceiver: no mapping found for destination ip %s\n", Utils::ToString(ipHeader->dest_ip).c_str());
		return;
	}

	// wait for handshake
	if (!this->cv.wait_for(this->lock, std::chrono::seconds(15), [this] { return this->mtx_ready; })) {
		// timeout
		throw std::runtime_error("Wait for handshake error");
	}

	try {
		this->steamMessageProcessor.SendMessage(it->second, message, size);
	}
	catch (const std::exception& ex) {
		DebugLog("Client::TUNDataReceiver: exception when sending steam message: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Client::TUNDataReceiver: unknown exception when sending steam message\n");
	}
}

void Client::SteamMessageReceiver(CSteamID userId, const char* message, size_t size) {
    // send to tun
	if (message == nullptr) {
		DebugLog("Client::SteamMessageReceiver: null message received from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (size == 0) {
		DebugLog("Client::SteamMessageReceiver: zero-size message from %llu\n", userId.ConvertToUint64());
		return;
	}

	try {
		this->tunMessageProcessor.SendData(message, size);
	}
	catch (const std::exception& ex) {
		DebugLog("Client::SteamMessageReceiver: exception when sending data to TUN: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Client::SteamMessageReceiver: unknown exception when sending data to TUN\n");
	}
}

void Client::SteamSystemMessageReceiver(CSteamID userId, const char* message, size_t size) {
	DebugLog("Client::SteamSystemMessageReceiver\n");
	Utils::PrintBytes(message, size);
    // receive handshake
	if (message == nullptr) {
		DebugLog("Client::SteamSystemMessageReceiver: null message from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (size == 0) {
		DebugLog("Client::SteamSystemMessageReceiver: zero-size system message from %llu\n", userId.ConvertToUint64());
		return;
	}

	uint8_t type = static_cast<uint8_t>(message[0]);
	if (type == 2) {
		DebugLog("Client::SteamSystemMessageReceiver: handshake message\n");

		if (size < sizeof(system_handshake_message)) {
			DebugLog("Client::SteamSystemMessageReceiver: handshake message too small (%zu) from %llu\n", size, userId.ConvertToUint64());
			return;
		}
		auto handshakeMessage = reinterpret_cast<const system_handshake_message*>(message);
		try {
			DebugLog("Client::SteamSystemMessageReceiver: received ip: %u, %s\n", handshakeMessage->ip, Utils::ToString(handshakeMessage->ip).c_str());
			this->callback(handshakeMessage->ip);
			this->tunMessageProcessor.Start(handshakeMessage->ip);
			this->mtx_ready = true;
			this->cv.notify_all();
		}
		catch (const std::exception& ex) {
			DebugLog("Client::SteamSystemMessageReceiver: exception in callback: %s\n", ex.what());
		}
		catch (...) {
			DebugLog("Client::SteamSystemMessageReceiver: unknown exception in callback\n");
		}
		return;
	}

	if (type == 3) { // system_error_message
		DebugLog("Client::SteamSystemMessageReceiver: system error message from %llu\n", userId.ConvertToUint64());
		return;
	}
	DebugLog("Client::SteamSystemMessageReceiver: unexpected system message type %u from %llu (size=%zu)\n", type, userId.ConvertToUint64(), size);
}

void Client::SendPasswordMassage(SteamNetworkingIdentity user) {
    try {
		system_password_message message = MessageCreator::getpasswordMessage((uint8_t*)this->password.c_str());
		Utils::PrintBytes((char*)&message, sizeof(message));
		this->steamMessageProcessor.SendSystemMessage(user, (char*)&message, sizeof(message));
	}
	catch (const std::exception& ex) {
		DebugLog("Client::SendPasswordMassage: exception: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Client::SendPasswordMassage: unknown exception\n");
	}
}

void Client::SendErrorMassage(SteamNetworkingIdentity user, uint32_t errorCode) {
    try {
		system_error_message message = MessageCreator::getErrorMessage(errorCode);
		Utils::PrintBytes((char*)&message, sizeof(message));
		this->steamMessageProcessor.SendSystemMessage(user, (char*)&message, sizeof(message));
	}
	catch (const std::exception& ex) {
		DebugLog("Client::SendErrorMassage: exception: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Client::SendErrorMassage: unknown exception\n");
	}
}