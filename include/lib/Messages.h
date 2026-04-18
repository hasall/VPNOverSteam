#ifndef MESSAGES_H
#define MESSAGES_H

#include <cstdint>
#include <steam/steam_api_flat.h>

#pragma pack(1)
struct ip_header {
	uint8_t ihl : 4;     // IP header length
	uint8_t version : 4; // IP version
	uint8_t tos;         // Type of Service
	uint16_t total_len;  // Total length
	uint16_t id;         // Identification
	uint16_t frag_off;   // Fragment offset
	uint8_t ttl;         // Time to live
	uint8_t protocol;    // Next level protocol
	uint16_t checksum;   // Header checksum
	uint32_t src_ip;     // Source IP address
	uint32_t dest_ip;    // Destination IP address
};

#pragma pack(1)
struct system_password_message {
	uint8_t type = 1;
	uint8_t password[SHA512_SIZE]; // SHA-2 512 hash
	uint8_t version;
};

#pragma pack(1)
struct system_handshake_message {
	uint8_t type = 2;
	uint32_t ip;
};

#pragma pack(1)
struct system_error_message {
	uint8_t type = 3;
	uint32_t errorCode;
};

#pragma pack(1)
struct system_connected_member_message {
	uint8_t type = 4;
	uint64 userId;
	uint32_t ip;
};

#pragma pack(1)
struct system_disconnected_member_message {
	uint8_t type = 5;
	uint64 userId;
	uint32_t ip;
};

#pragma pack(1)
struct system_ping_message {
	uint8_t type = 6;
};

#pragma pack(1)
struct system_pong_message {
	uint8_t type = 7;
};

#define ERROR_CODE_WRONG_PASSWORD 1

class MessageCreator {
public:
	static system_password_message getpasswordMessage(uint8_t* password) {
		system_password_message message = { 1 };
		memcpy(message.password, password, SHA512_SIZE);
		message.version = APP_VERSION;
		return message;
	}

	static system_handshake_message getHandshakeMessage(uint32_t ip) {
		system_handshake_message message = { 2, ip };
		return message;
	}

	static system_error_message getErrorMessage(uint32_t errorCode) {
		system_error_message message = { 3, errorCode };
		return message;
	}

	static system_connected_member_message getConnectedMemberMessage(uint64 userId, uint32_t ip) {
		system_connected_member_message message = { 4, userId, ip };
		return message;
	}

	static system_disconnected_member_message getDisconnectedMemberMessage(uint64 userId, uint32_t ip) {
		system_disconnected_member_message message = { 5, userId, ip };
		return message;
	}

	static system_ping_message getPingMessage() {
		system_ping_message message = { 6 };
		return message;
	}

	static system_pong_message getPongMessage() {
		system_pong_message message = { 7 };
		return message;
	}
};

#endif // MESSAGES_H
