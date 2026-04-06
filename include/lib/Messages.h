#ifndef MESSAGES_H
#define MESSAGES_H

#include <cstdint>

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

#define ERROR_CODE_WRONG_PASSWORD 1

class MessageCreator {
public:
	static system_password_message getpasswordMessage(uint8_t* password) {
		system_password_message message = { 1 };
		memcpy(message.password, password, SHA512_SIZE);
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
};

#endif // MESSAGES_H
