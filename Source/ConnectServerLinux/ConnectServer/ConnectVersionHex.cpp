#include "StdAfx.h"
#include "Util.h"
#include "ConnectVersionHex.h"
#include "RuntimeContext.h"

int ServerVersion = 0;

std::vector<ConnectVersionHex> VersionHex;

ConnectVersionHex::ConnectVersionHex()
{
	Version = 0;
}

ConnectVersionHex::~ConnectVersionHex()
{
	buffer.clear();
}

int ConnectVersionHex::GetVersion()
{
	return Version;
}

size_t ConnectVersionHex::GetFileSize()
{
	return buffer.size();
}

std::vector<BYTE> ConnectVersionHex::GetFileBuffer()
{
	return buffer;
}

int ConnectVersionHex::OpenDataVersion(int version, std::string filename)
{
	Version = version;

	FILE* fp = fopen(filename.c_str(), "rb");

	if (fp == NULL)
	{
		return (-1);
	}

	fseek(fp, 0, SEEK_END);

	unsigned int FileSize = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	buffer = std::vector<BYTE>(FileSize, 0);

	fread(buffer.data(), 1u, FileSize, fp);

	fclose(fp);

	return FileSize;
}

void LoadServerVersion()
{
	VersionHex.clear();

	ServerVersion = ReadPrivateProfileInt("ConnectServerInfo", "ConnectServerVersion", 0, GetConnectServerIniPath());

	const std::filesystem::path updateDirectory = GetUpdateDirectoryPath();
	if (std::filesystem::exists(updateDirectory) == false)
	{
		return;
	}

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(updateDirectory))
	{
		if (entry.is_directory() != false)
		{
			continue;
		}

		const std::string fileName = entry.path().filename().string();
		if (fileName.size() < 6)
		{
			continue;
		}

		if (std::isdigit(static_cast<unsigned char>(fileName[0])) == 0 ||
			std::isdigit(static_cast<unsigned char>(fileName[1])) == 0 ||
			std::isdigit(static_cast<unsigned char>(fileName[2])) == 0)
		{
			continue;
		}

		if (fileName[3] != ' ' || fileName[4] != '-' || fileName[5] != ' ')
		{
			continue;
		}

		int version = std::atoi(fileName.c_str());
		ConnectVersionHex info;
		size_t totalSize = info.OpenDataVersion(version, entry.path().string());

		if (totalSize > 0)
		{
			VersionHex.push_back(info);
		}
	}

	LogAdd(LOG_BLUE, (char*)"[ServerVersion] ServerVersion loaded successfully");
}

std::vector<BYTE> GetServerVersion(int version)
{
	std::vector<BYTE> EncData;

	for (size_t i = 0; i < VersionHex.size(); i++)
	{
		if (VersionHex[i].GetVersion() == version)
		{
			EncData = VersionHex[i].GetFileBuffer();

			if (!EncData.empty())
			{
				return EncData;
			}
		}
	}

	return EncData;
}
