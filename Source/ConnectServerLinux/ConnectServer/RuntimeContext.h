#pragma once

struct CONNECT_SERVER_RUNTIME_PATHS
{
	std::filesystem::path RootDirectory;
	std::filesystem::path DataDirectory;
	std::filesystem::path ConnectServerIni;
	std::filesystem::path ServerListDat;
	std::filesystem::path UpdateDirectory;
	std::filesystem::path LogDirectory;
};

struct CONNECT_SERVER_RUNTIME_CONFIG
{
	WORD ConnectServerPortTCP;
	WORD ConnectServerPortUDP;
	long MaxIpConnection;
	long MaxHWIDConnection;
	int ConnectServerVersion;
	std::string CustomerName;
	std::string CustomerHardwareId;
};

bool InitializeRuntimeContext(const char* argv0);
const CONNECT_SERVER_RUNTIME_PATHS& GetRuntimePaths();
const CONNECT_SERVER_RUNTIME_CONFIG& GetRuntimeConfig();
bool ReloadRuntimeConfig(bool logSummary);

const char* GetConnectServerIniPath();
const char* GetServerListDataPath();
const char* GetUpdateDirectoryPath();
const char* GetLogDirectoryPath();

std::string ReadPrivateProfileString(const char* section, const char* key, const char* defaultValue, const char* filePath);
int ReadPrivateProfileInt(const char* section, const char* key, int defaultValue, const char* filePath);
