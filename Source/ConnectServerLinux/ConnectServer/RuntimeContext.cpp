#include "stdafx.h"
#include "RuntimeContext.h"

namespace
{
	CONNECT_SERVER_RUNTIME_PATHS gRuntimePaths;
	CONNECT_SERVER_RUNTIME_CONFIG gRuntimeConfig = {44405, 55557, 0, 1, 0, "", ""};

	std::string gConnectServerIniPath;
	std::string gServerListDataPath;
	std::string gUpdateDirectoryPath;
	std::string gLogDirectoryPath;

	std::filesystem::path ResolveExecutablePath(const char* argv0)
	{
#ifdef __linux__
		std::array<char, MAX_PATH> buffer = {0};
		const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
		if (length > 0) {
			buffer[static_cast<std::size_t>(length)] = 0;
			return std::filesystem::path(buffer.data());
		}
#endif

		if (argv0 != nullptr && std::strlen(argv0) > 0) {
			return std::filesystem::absolute(argv0);
		}

		return std::filesystem::current_path() / "connectserver";
	}

	std::map<std::string, std::string> ParseIniSection(const char* requestedSection, const char* filePath)
	{
		std::ifstream input(filePath);
		std::map<std::string, std::string> values;

		if (!input.is_open()) {
			return values;
		}

		const std::string sectionName = ToLowerCopy(requestedSection != nullptr ? requestedSection : "");
		std::string currentSection;
		std::string line;

		while (std::getline(input, line)) {
			line = TrimString(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') {
				continue;
			}

			if (line.front() == '[' && line.back() == ']') {
				currentSection = ToLowerCopy(TrimString(line.substr(1, line.size() - 2)));
				continue;
			}

			if (currentSection != sectionName) {
				continue;
			}

			const std::size_t separator = line.find('=');
			if (separator == std::string::npos) {
				continue;
			}

			const std::string key = ToLowerCopy(TrimString(line.substr(0, separator)));
			const std::string value = TrimString(line.substr(separator + 1));
			values[key] = value;
		}

		return values;
	}

	void ApplyStringToFixedBuffer(const std::string& value, char* buffer, std::size_t bufferSize)
	{
		if (buffer == nullptr || bufferSize == 0) {
			return;
		}

		std::memset(buffer, 0, bufferSize);
		std::snprintf(buffer, bufferSize, "%s", value.c_str());
	}
}

bool InitializeRuntimeContext(const char* argv0)
{
	const std::filesystem::path executablePath = ResolveExecutablePath(argv0);
	gRuntimePaths.RootDirectory = executablePath.parent_path();
	gRuntimePaths.DataDirectory = gRuntimePaths.RootDirectory / "Data";
	gRuntimePaths.ConnectServerIni = gRuntimePaths.DataDirectory / "ConnectServer.ini";
	gRuntimePaths.ServerListDat = gRuntimePaths.DataDirectory / "ServerList.dat";
	gRuntimePaths.UpdateDirectory = gRuntimePaths.DataDirectory / "Update";
	gRuntimePaths.LogDirectory = gRuntimePaths.RootDirectory / "LOG";

	gConnectServerIniPath = gRuntimePaths.ConnectServerIni.string();
	gServerListDataPath = gRuntimePaths.ServerListDat.string();
	gUpdateDirectoryPath = gRuntimePaths.UpdateDirectory.string();
	gLogDirectoryPath = gRuntimePaths.LogDirectory.string();

	std::error_code errorCode;
	std::filesystem::create_directories(gRuntimePaths.LogDirectory, errorCode);
	return ReloadRuntimeConfig(false);
}

const CONNECT_SERVER_RUNTIME_PATHS& GetRuntimePaths()
{
	return gRuntimePaths;
}

const CONNECT_SERVER_RUNTIME_CONFIG& GetRuntimeConfig()
{
	return gRuntimeConfig;
}

bool ReloadRuntimeConfig(bool logSummary)
{
	const std::string iniPath = gConnectServerIniPath;
	if (iniPath.empty() || std::filesystem::exists(iniPath) == false) {
		return false;
	}

	gRuntimeConfig.CustomerName = ReadPrivateProfileString("ConnectServerInfo", "CustomerName", "", iniPath.c_str());
	gRuntimeConfig.CustomerHardwareId = ReadPrivateProfileString("ConnectServerInfo", "CustomerHardwareId", "", iniPath.c_str());
	gRuntimeConfig.ConnectServerPortTCP = static_cast<WORD>(ReadPrivateProfileInt("ConnectServerInfo", "ConnectServerPortTCP", 44405, iniPath.c_str()));
	gRuntimeConfig.ConnectServerPortUDP = static_cast<WORD>(ReadPrivateProfileInt("ConnectServerInfo", "ConnectServerPortUDP", 55557, iniPath.c_str()));
	gRuntimeConfig.MaxIpConnection = ReadPrivateProfileInt("ConnectServerInfo", "MaxIpConnection", 0, iniPath.c_str());
	gRuntimeConfig.MaxHWIDConnection = ReadPrivateProfileInt("ConnectServerInfo", "MaxHWIDConnection", 1, iniPath.c_str());
	gRuntimeConfig.ConnectServerVersion = ReadPrivateProfileInt("ConnectServerInfo", "ConnectServerVersion", 0, iniPath.c_str());

	ApplyStringToFixedBuffer(gRuntimeConfig.CustomerName, CustomerName, sizeof(CustomerName));
	ApplyStringToFixedBuffer(gRuntimeConfig.CustomerHardwareId, CustomerHardwareId, sizeof(CustomerHardwareId));
	MaxIpConnection = gRuntimeConfig.MaxIpConnection;
	MaxHWIDConnection = gRuntimeConfig.MaxHWIDConnection;

	if (logSummary) {
		std::cout << "[Config] ConnectServer.ini recarregado de " << iniPath << std::endl;
	}

	return true;
}

const char* GetConnectServerIniPath()
{
	return gConnectServerIniPath.c_str();
}

const char* GetServerListDataPath()
{
	return gServerListDataPath.c_str();
}

const char* GetUpdateDirectoryPath()
{
	return gUpdateDirectoryPath.c_str();
}

const char* GetLogDirectoryPath()
{
	return gLogDirectoryPath.c_str();
}

std::string ReadPrivateProfileString(const char* section, const char* key, const char* defaultValue, const char* filePath)
{
	const std::map<std::string, std::string> values = ParseIniSection(section, filePath);
	const auto iterator = values.find(ToLowerCopy(key != nullptr ? key : ""));
	if (iterator == values.end()) {
		return (defaultValue != nullptr) ? defaultValue : "";
	}

	return iterator->second;
}

int ReadPrivateProfileInt(const char* section, const char* key, int defaultValue, const char* filePath)
{
	const std::string value = ReadPrivateProfileString(section, key, "", filePath);
	if (value.empty()) {
		return defaultValue;
	}

	try {
		return std::stoi(value);
	}
	catch (...) {
		return defaultValue;
	}
}
