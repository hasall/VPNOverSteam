#ifndef UTILS_H
#define UTILS_H

#include <string>

class Utils {
public:
	// SHA-512 (SHA2-512) hash of input string, returned as lowercase hex
	static std::string SHA512(const std::string& str);
	static std::string ToString(uint32_t ip);
	static uint32_t FromString(const std::string& ip);
	static void PrintBytes(const char* message, size_t size);
};

#endif // UTILS_H
