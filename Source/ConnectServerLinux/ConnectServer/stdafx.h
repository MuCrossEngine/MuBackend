#pragma once

#define CONNECTSERVER_VERSION "CS"
#define CONNECTSERVER_CLIENT "MGM EMULATOR"

#ifndef CONNECTSERVER_UPDATE
#define CONNECTSERVER_UPDATE 803
#endif

#ifndef PROTECT_STATE
#define PROTECT_STATE 0
#endif

#define SOCKET_LAUNCHER

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <condition_variable>

#ifdef __linux__
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using QWORD = std::uint64_t;
using UINT = std::uint32_t;
using BOOL = int;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef WINAPI
#define WINAPI
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef FAR
#define FAR
#endif

#ifdef __linux__
using SOCKET = int;
using HANDLE = int;
using SOCKADDR_IN = sockaddr_in;
using SOCKADDR = sockaddr;
using HOSTENT = hostent;

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#define closesocket close
#endif

#ifndef MAKEWORD
#define MAKEWORD(low, high) ((WORD)(((BYTE)((low) & 0xFF)) | ((WORD)((BYTE)((high) & 0xFF)) << 8)))
#endif

inline DWORD GetTickCount() {
	static const auto start = std::chrono::steady_clock::now();
	return static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count());
}

inline void SleepMs(DWORD milliseconds) {
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline int localtime_s(std::tm* output, const std::time_t* input) {
#ifdef _WIN32
	return ::localtime_s(output, input);
#else
	return (localtime_r(input, output) == nullptr) ? errno : 0;
#endif
}

inline int asctime_s(char* buffer, std::size_t size, const std::tm* timeInfo) {
	char temp[32] = {0};
#ifdef _WIN32
	if (::asctime_s(temp, sizeof(temp), timeInfo) != 0) {
		return 1;
	}
#else
	if (asctime_r(timeInfo, temp) == nullptr) {
		return 1;
	}
#endif

	if (std::strlen(temp) + 1 > size) {
		return 1;
	}

	std::memcpy(buffer, temp, std::strlen(temp) + 1);
	return 0;
}

inline int strcpy_s(char* destination, std::size_t destinationSize, const char* source) {
	if (destination == nullptr || source == nullptr || destinationSize == 0) {
		return 1;
	}

	std::snprintf(destination, destinationSize, "%s", source);
	return 0;
}

template <std::size_t N>
inline int strcpy_s(char (&destination)[N], const char* source) {
	return strcpy_s(destination, N, source);
}

template <std::size_t N>
inline int vsprintf_s(char (&buffer)[N], const char* format, va_list args) {
	return std::vsnprintf(buffer, N, format, args);
}

template <std::size_t N>
inline int sprintf_s(char (&buffer)[N], const char* format, ...) {
	va_list args;
	va_start(args, format);
	const int result = std::vsnprintf(buffer, N, format, args);
	va_end(args);
	return result;
}

inline std::string TrimString(const std::string& value) {
	std::size_t start = 0;
	while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
		++start;
	}

	std::size_t end = value.size();
	while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
		--end;
	}

	return value.substr(start, end - start);
}

inline std::string ToLowerCopy(const std::string& value) {
	std::string result = value;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return result;
}

extern char CustomerName[32];
extern char CustomerHardwareId[36];
extern long MaxIpConnection;
extern long MaxHWIDConnection;
