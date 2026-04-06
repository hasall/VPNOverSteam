#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <random>


class Config {
public:
	// Inline static members provide definitions in the header and sensible defaults
#ifdef _WIN32
	inline static GUID AdapterGuid;
#endif
	inline static std::string ServerIp = "10.0.0.1";
	inline static std::string IpPoolFrom = "10.0.0.100";
	inline static std::string IpPoolTo = "10.0.0.200";

	inline static std::string LobbyGameNameKey = "GameName";
	inline static std::string LobbyGameNameValue = "VPNOverSteam";
	inline static std::string LobbyNameKey = "LobbyName";
	inline static std::string LobbyPasswordKey = "Password";
	inline static std::string LobbyUserNameKey = "UserName";
	inline static std::string LobbyUserIpKey = "UserIP";
	inline static std::string UnreadableName = "(null)";

	static void Init() {
		std::random_device rd;
		std::mt19937 gen(rd());

#ifdef _WIN32
		AdapterGuid = { 0xdeadbabe, 0xcafe, 0xbeef, { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, (unsigned char) ((gen() % 250) + 2), (unsigned char)((gen() % 250) + 2) } };
#endif

		Config::Load();
		Config::Save();
	}

private:

	static void Load() {
		std::ifstream file("config.txt");
		if (!file.is_open()) {
			std::cerr << "Config file not found. Creating default config.txt\n";
			return;
		}

		auto trim = [](std::string &s) {
			size_t p1 = s.find_first_not_of(" \t\r\n");
			if (p1 == std::string::npos) { s.clear(); return; }
			size_t p2 = s.find_last_not_of(" \t\r\n");
			s = s.substr(p1, p2 - p1 + 1);
		};

		std::string line;
		while (std::getline(file, line)) {
			if (line.empty()) continue;
			if (line.front() == '#') continue;
			auto pos = line.find('=');
			if (pos == std::string::npos) continue;
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			trim(key);
			trim(value);
			if (key == "ServerIp") ServerIp = value;
			else if (key == "IpPoolFrom") IpPoolFrom = value;
			else if (key == "IpPoolTo") IpPoolTo = value;

#ifdef _WIN32
			else if (key == "AdapterGuid") {
				int countOfGUIDParts = 0;
				unsigned long guidParts[11] = { 0 };

				for (countOfGUIDParts = 0; countOfGUIDParts < _countof(guidParts); countOfGUIDParts++) {
					auto pos = value.find(':');
					std::string data;

					if (pos != std::string::npos) {
						data = value.substr(0, pos);
						value = value.substr(pos + 1);
					}
					else {
						if (countOfGUIDParts + 1 != _countof(guidParts)) break;
						data = value;
					}

					guidParts[countOfGUIDParts] = std::stoul(data, nullptr, 16);
				}
				if (countOfGUIDParts < _countof(guidParts)) {
					std::cerr << "Invalid AdapterGuid format in config.txt, expected " << _countof(guidParts) << " parts separated by ':', got " << countOfGUIDParts << "\n";
					continue;
				}

				AdapterGuid = { 
					(unsigned long)guidParts[0], 
					(unsigned short)guidParts[1], 
					(unsigned short)guidParts[2], 
					{ 
						(unsigned char)guidParts[3],
						(unsigned char)guidParts[4],
						(unsigned char)guidParts[5],
						(unsigned char)guidParts[6],
						(unsigned char)guidParts[7],
						(unsigned char)guidParts[8],
						(unsigned char)guidParts[9],
						(unsigned char)guidParts[10],
					} 
				};
			}
#endif

			else if (key == "LobbyGameNameKey") LobbyGameNameKey = value;
			else if (key == "LobbyGameNameValue") LobbyGameNameValue = value;
			else if (key == "LobbyNameKey") LobbyNameKey = value;
			else if (key == "LobbyPasswordKey") LobbyPasswordKey = value;
			else if (key == "LobbyUserNameKey") LobbyUserNameKey = value;
			else if (key == "LobbyUserIpKey") LobbyUserIpKey = value;
			else if (key == "UnreadableName") UnreadableName = value;
		}

		file.close();
	}

	static void Save() {
		std::ofstream file("config.txt", std::ios::trunc);
		if (!file.is_open()) {
			std::cerr << "Failed to save config to config.txt\n";
			return;
		}
		file << "# VPNOverSteam configuration\n";
		file << "# Lines in the form Key=Value\n";

#ifdef _WIN32
		file << "# AdapterGuid = ulong:ushort:ushort:uchar:uchar:uchar:uchar:uchar:uchar:uchar:uchar\n";
		file << "AdapterGuid="
			<< std::uppercase
			<< std::hex
			<< AdapterGuid.Data1 << ":"
			<< AdapterGuid.Data2 << ":"
			<< AdapterGuid.Data3 << ":"
			<< (int)AdapterGuid.Data4[0] << ":"
			<< (int)AdapterGuid.Data4[1] << ":"
			<< (int)AdapterGuid.Data4[2] << ":"
			<< (int)AdapterGuid.Data4[3] << ":"
			<< (int)AdapterGuid.Data4[4] << ":"
			<< (int)AdapterGuid.Data4[5] << ":"
			<< (int)AdapterGuid.Data4[6] << ":"
			<< (int)AdapterGuid.Data4[7]
			<< "\n";
#endif

		file << "ServerIp=" << ServerIp << "\n";
		file << "IpPoolFrom=" << IpPoolFrom << "\n";
		file << "IpPoolTo=" << IpPoolTo << "\n";

		file << "LobbyGameNameKey=" << LobbyGameNameKey << "\n";
		file << "LobbyGameNameValue=" << LobbyGameNameValue << "\n";
		file << "LobbyNameKey=" << LobbyNameKey << "\n";
		file << "LobbyPasswordKey=" << LobbyPasswordKey << "\n";
		file << "LobbyUserNameKey=" << LobbyUserNameKey << "\n";
		file << "LobbyUserIpKey=" << LobbyUserIpKey << "\n";
		file << "UnreadableName=" << UnreadableName << "\n";

		file.close();
	}
};

#endif // CONFIG_H
