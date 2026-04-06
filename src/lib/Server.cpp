#include <algorithm>
#include <functional>
#include <cstdint>

#include "lib/Server.h"
#include "lib/DebugLog.h"
#include "lib/Constants.h"
#include "lib/Messages.h"
#include "lib/Utils.h"
#include "lib/Config.h"

#ifdef _WIN32
Server::Server(GUID guid) :
#else
Server::Server() :
#endif

	password(),
	ipPool(Config::IpPoolFrom, Config::IpPoolTo),
	tunMessageProcessor(
		std::bind(&Server::TUNDataReceiver, this, std::placeholders::_1, std::placeholders::_2)
		#ifdef _WIN32
		, guid
		#endif
	),
	steamMessageProcessor(
		std::bind(&Server::SteamMessageReceiver, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&Server::SteamSystemMessageReceiver, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	)
{}

Server::~Server() {
	this->Stop();
}

void Server::JoinMember(uint64 userId, const char* userName, uint32_t ip) {
    DebugLog("Server::JoinMember id: %llu, name: %s, ip: %s\n", userId, userName, Utils::ToString(ip).c_str());
}

void Server::LeftMember(uint64 userId) {
    DebugLog("Server::LeftMember id: %llu\n", userId);
	try {
		auto it = std::find_if(this->ipToClient.begin(), this->ipToClient.end(),
			[&userId](const std::pair<uint32_t, SteamNetworkingIdentity>& pair) {
				return pair.second.GetSteamID() == userId;
			});
		if (it == this->ipToClient.end()) {
			DebugLog("Server::LeftMember: userId %llu not found in map\n", userId);
		} 
		else {
			SteamNetworkingMessages()->CloseSessionWithUser(it->second);
			this->ipPool.Release(it->first);
			this->ipToClient.erase(it);
		}
	}
	catch (const std::exception& ex) {
		DebugLog("Server::LeftMember: exception: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Server::LeftMember: unknown exception\n");
	}
}

void Server::Start(std::string password) {
	this->password = Utils::SHA512(password);
	this->steamMessageProcessor.Start();
	this->tunMessageProcessor.Start(Utils::FromString(Config::ServerIp));
}

void Server::Stop() {
	this->steamMessageProcessor.Stop();
	this->tunMessageProcessor.Stop();
}

void Server::TUNDataReceiver(const char* message, size_t size) {
    // send to steam
	if (message == nullptr) {
		DebugLog("Server::TUNDataReceiver: null message received\n");
		return;
	}
	if (size < sizeof(ip_header)) {
		DebugLog("Server::TUNDataReceiver: message size too small (%zu)\n", size);
		return;
	}
	auto ipHeader = reinterpret_cast<const ip_header*>(message);
	auto it = this->ipToClient.find(ipHeader->dest_ip);
	if (it == this->ipToClient.end()) {
		DebugLog("Server::TUNDataReceiver: no mapping found for destination ip %s\n", Utils::ToString(ipHeader->dest_ip).c_str());
		return;
	}

	try {
		this->steamMessageProcessor.SendMessage(it->second, message, size);
	}
	catch (const std::exception& ex) {
		DebugLog("Server::TUNDataReceiver: exception when sending steam message: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Server::TUNDataReceiver: unknown exception when sending steam message\n");
	}
}

void Server::SteamMessageReceiver(CSteamID userId, const char* message, size_t size) {
    // send to tun
	if (message == nullptr) {
		DebugLog("Server::SteamMessageReceiver: null message received from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (size == 0) {
		DebugLog("Server::SteamMessageReceiver: zero-size message from %llu\n", userId.ConvertToUint64());
		return;
	}
	try {
		this->tunMessageProcessor.SendData(message, size);
	}
	catch (const std::exception& ex) {
		DebugLog("Server::SteamMessageReceiver: exception when sending data to TUN: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Server::SteamMessageReceiver: unknown exception when sending data to TUN\n");
	}
}

void Server::SteamSystemMessageReceiver(CSteamID userId, const char* message, size_t size) {
	DebugLog("Server::SteamSystemMessageReceiver\n");
	Utils::PrintBytes(message, size);
    // send handshake
	if (message == nullptr) {
		DebugLog("Server::SteamSystemMessageReceiver: null message from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (size == 0) {
		DebugLog("Server::SteamSystemMessageReceiver: zero-size system message from %llu\n", userId.ConvertToUint64());
		return;
	}
	uint8_t type = static_cast<uint8_t>(message[0]);
	if (type == 1) {
		DebugLog("Server::SteamSystemMessageReceiver: password message\n");

		if (size < sizeof(system_password_message)) {
			DebugLog("Server::SteamSystemMessageReceiver: password message too small (%zu) from %llu\n", size, userId.ConvertToUint64());
			return;
		}

		auto passwordMessage = reinterpret_cast<const system_password_message*>(message);
		auto user = SteamNetworkingIdentity();
		user.SetSteamID(userId);

		try {
			// check password and send handshake
			auto pass = std::string(passwordMessage->password, passwordMessage->password + SHA512_SIZE);
			if (this->password.compare(pass) == 0) {
				auto ip = ipPool.Allocate();
				this->ipToClient.insert({ ip, user });
				DebugLog("Server::SteamSystemMessageReceiver: send ip: %u, %s\n", ip, Utils::ToString(ip).c_str());
				this->SendHandshakeMassage(user, ip);
				return;
			}
			DebugLog("Server::SteamSystemMessageReceiver: Incorrect password\n");
			this->SendErrorMassage(user, ERROR_CODE_WRONG_PASSWORD);
		}
		catch (const std::exception& ex) {
			DebugLog("Server::SteamSystemMessageReceiver: exception handling password: %s\n", ex.what());
		}
		catch (...) {
			DebugLog("Server::SteamSystemMessageReceiver: unknown exception handling password\n");
		}
	}

	if (type == 3) { // system_error_message
		DebugLog("Server::SteamSystemMessageReceiver: system error message from %llu\n", userId.ConvertToUint64());
		return;
	}
	DebugLog("Server::SteamSystemMessageReceiver: unexpected system message\n");
}

void Server::SendHandshakeMassage(SteamNetworkingIdentity user, uint32_t ip) {
	try {
		system_handshake_message message = MessageCreator::getHandshakeMessage(ip);
		Utils::PrintBytes((char*)&message, sizeof(message));
		this->steamMessageProcessor.SendSystemMessage(user, (char*)&message, sizeof(message));
	}
	catch (const std::exception& ex) {
		DebugLog("Server::SendErrorMassage: exception: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Server::SendErrorMassage: unknown exception\n");
	}
}

void Server::SendErrorMassage(SteamNetworkingIdentity user, uint32_t errorCode) {
	try {
		system_error_message message = MessageCreator::getErrorMessage(errorCode);
		Utils::PrintBytes((char*)&message, sizeof(message));
		this->steamMessageProcessor.SendSystemMessage(user, (char*)&message, sizeof(message));
	}
	catch (const std::exception& ex) {
		DebugLog("Server::SendErrorMassage: exception: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Server::SendErrorMassage: unknown exception\n");
	}
}