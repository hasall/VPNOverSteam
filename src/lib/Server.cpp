#include <algorithm>
#include <functional>
#include <cstdint>

#include "lib/Server.h"
#include "lib/DebugLog.h"
#include "lib/Constants.h"
#include "lib/Messages.h"
#include "lib/Utils.h"
#include "lib/Config.h"

Server::Server() :
	password(),
	ipPool(Config::IpPoolFrom, Config::IpPoolTo),
	tunMessageProcessor(
		std::bind(&Server::TUNDataReceiver, this, std::placeholders::_1, std::placeholders::_2)
	),
	steamMessageProcessor(
		std::bind(&Server::SteamMessageReceiver, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&Server::SteamSystemMessageReceiver, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	),
	running(false)
{}

Server::~Server() {
	this->Stop();
}

void Server::JoinMember(uint64 userId, uint32_t ip) {
    DebugLog("Server::JoinMember id: %llu, ip: %s\n", userId, Utils::ToString(ip).c_str());
}

void Server::LeftMember(uint64 userId) {
    DebugLog("Server::LeftMember id: %llu\n", userId);
	try {
		auto it = std::find_if(this->usersList.begin(), this->usersList.end(),
			[&userId](const user_info& i) {
				return i.identity.GetSteamID().ConvertToUint64() == userId;
			});
		if (it == this->usersList.end()) {
			DebugLog("Server::LeftMember: userId %llu not found in map\n", userId);
		} 
		else {
			SteamAPI_ISteamNetworkingMessages_CloseSessionWithUser(SteamAPI_SteamGameServerNetworkingMessages_SteamAPI(), it->identity);
			this->ipPool.Release(it->ip);
			this->usersList.erase(it);
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
	if (this->running == true) {
		DebugLog("Server::Start: already running\n");
		return;
	}
	this->password = Utils::SHA512(password);
	this->steamMessageProcessor.SetEncryptionKey(this->password);
	this->steamMessageProcessor.Start();
	this->tunMessageProcessor.Start(Utils::FromString(Config::ServerIp));

	this->running = true;
	this->pingThread = std::thread(&Server::PingLoop, this);
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
	auto it = std::find_if(this->usersList.begin(), this->usersList.end(), 
		[&ipHeader](const user_info& i) {
			return i.ip == ipHeader->dest_ip;
		});
	if (it == this->usersList.end()) {
		DebugLog("Server::TUNDataReceiver: no mapping found for destination ip %s\n", Utils::ToString(ipHeader->dest_ip).c_str());
		return;
	}

	try {
		this->steamMessageProcessor.SendMessage(it->identity, message, size);
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
		// check if user is in map
		auto it = std::find_if(this->usersList.begin(), this->usersList.end(),
			[&userId](const user_info& i) {
				return i.identity.GetSteamID() == userId;
			});
		if (it == this->usersList.end()) {
			DebugLog("Server::SteamMessageReceiver: warning: userId %llu not found in map\n", userId.ConvertToUint64());
			return;
		}
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
				DebugLog("Server::SteamSystemMessageReceiver: send ip: %u, %s\n", ip, Utils::ToString(ip).c_str());

				this->SendHandshakeMassage(user, ip);
				for (const auto& i : this->usersList) {
					this->SendNotifyOfNewMemberMessage(i.identity, userId.ConvertToUint64(), ip);
				}
				this->JoinMember(userId.ConvertToUint64(), ip);

				this->usersList.push_back({ user, ip });
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
	if (type == 2) { // unexpected handshake message
		DebugLog("Server::SteamSystemMessageReceiver: unexpected handshake message from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (type == 3) { // system_error_message
		DebugLog("Server::SteamSystemMessageReceiver: system error message from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (type == 4) { // unexpected new member message
		DebugLog("Server::SteamSystemMessageReceiver: unexpected generic message from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (type == 5) { // unexpected disconnected member message
		DebugLog("Server::SteamSystemMessageReceiver: unexpected disconnected member message from %llu\n", userId.ConvertToUint64());
		return;
	}
	if (type == 6) { // ping message
		DebugLog("Server::SteamSystemMessageReceiver: ping message from %llu\n", userId.ConvertToUint64());
		try {
			auto user = SteamNetworkingIdentity();
			user.SetSteamID(userId);
			system_pong_message message = MessageCreator::getPongMessage();
			this->steamMessageProcessor.SendSystemMessage(user, (char*)&message, sizeof(message));

			auto userInfo = std::find_if(this->usersList.begin(), this->usersList.end(),
				[&userId](const user_info& info) {
					return info.identity.GetSteamID() == userId;
				});
			if (userInfo != this->usersList.end()) {
				userInfo->lastPingTime = std::chrono::system_clock::now();
			}
		}
		catch (const std::exception& ex) {
			DebugLog("Server::SteamSystemMessageReceiver: exception handling ping message: %s\n", ex.what());
		}
		catch (...) {
			DebugLog("Server::SteamSystemMessageReceiver: unknown exception handling ping message\n");
		}
		return;
	}
	if (type == 7) { // pong message
		DebugLog("Server::SteamSystemMessageReceiver: pong message from %llu\n", userId.ConvertToUint64());
		auto userInfo = std::find_if(this->usersList.begin(), this->usersList.end(),
			[&userId](const user_info& info) {
				return info.identity.GetSteamID() == userId;
			});
		if (userInfo != this->usersList.end()) {
			userInfo->lastPingTime = std::chrono::system_clock::now();
		}
		return;
	}
	DebugLog("Server::SteamSystemMessageReceiver: unexpected system message\n");
}

void Server::SendHandshakeMassage(SteamNetworkingIdentity user, uint32_t ip) {
	DebugLog("Server::SendHandshakeMassage: sending handshake message to %llu, ip: %u, %s\n", user.GetSteamID().ConvertToUint64(), ip, Utils::ToString(ip).c_str());
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
	DebugLog("Server::SendErrorMassage: sending error message to %llu, errorCode: %u\n", user.GetSteamID().ConvertToUint64(), errorCode);
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

void Server::SendNotifyOfNewMemberMessage(SteamNetworkingIdentity user, uint64 userID, uint32_t ip) {
	DebugLog("Server::SendNotifyOfNewMemberMessage: sending new member message to %llu, userId: %llu, ip: %u, %s\n", user.GetSteamID().ConvertToUint64(), userID, ip, Utils::ToString(ip).c_str());
	try {
		system_connected_member_message message = MessageCreator::getConnectedMemberMessage(userID, ip);
		Utils::PrintBytes((char*)&message, sizeof(message));
		this->steamMessageProcessor.SendSystemMessage(user, (char*)&message, sizeof(message));
	}
	catch (const std::exception& ex) {
		DebugLog("Server::SendNewMemberMessage: exception: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("Server::SendNewMemberMessage: unknown exception\n");
	}
}

void Server::PingLoop() {
	std::vector<system_disconnected_member_message> listOfDisconnectedUsers;
	
	while (this->running == true) {
		std::this_thread::sleep_for(std::chrono::minutes(2));

		// remove if last ping time more than 5 min
		auto now = std::chrono::system_clock::now();
		if (listOfDisconnectedUsers.size() > 0) listOfDisconnectedUsers.clear();
		this->usersList.erase(
			std::remove_if(this->usersList.begin(), this->usersList.end(),
				[&now, &listOfDisconnectedUsers](const user_info& i) {
					auto res = i.lastPingTime < now - std::chrono::minutes(5);
					if (res) listOfDisconnectedUsers.push_back(MessageCreator::getDisconnectedMemberMessage(i.identity.GetSteamID().ConvertToUint64(), i.ip));
					return res;
				}),
			this->usersList.end()
		);

		// send ping for all users and notify on disconnected users
		for (const auto& userInfo : this->usersList) {
			try {
				system_ping_message pingMessage = MessageCreator::getPingMessage();
				this->steamMessageProcessor.SendSystemMessage(userInfo.identity, (char*)&pingMessage, sizeof(pingMessage));
				if (listOfDisconnectedUsers.size() > 0) {
					for (const auto& message: listOfDisconnectedUsers) {
						this->steamMessageProcessor.SendSystemMessage(userInfo.identity, (char*)&message, sizeof(message));
					}
				}
			}
			catch (const std::exception& ex) {
				DebugLog("Server::PingLoop: exception when sending ping to %llu: %s\n", userInfo.identity.GetSteamID().ConvertToUint64(), ex.what());
			}
			catch (...) {
				DebugLog("Server::PingLoop: unknown exception when sending ping to %llu\n", userInfo.identity.GetSteamID().ConvertToUint64());
			}
		}
	}
	DebugLog("Server::PingLoop stopped\n");
}