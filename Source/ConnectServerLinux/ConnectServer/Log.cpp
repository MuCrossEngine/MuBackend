// Log.cpp: implementation of the CLog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Log.h"

CLog gLog;

namespace
{
	bool OpenDailyLogFile(LOG_INFO* info)
	{
		if (info == nullptr)
		{
			return false;
		}

		const std::filesystem::path directoryPath(info->Directory);
		std::error_code errorCode;
		std::filesystem::create_directories(directoryPath, errorCode);

		std::time_t rawTime = std::time(nullptr);
		std::tm timeInfo;
		if (localtime_s(&timeInfo, &rawTime) != 0)
		{
			return false;
		}

		info->Day = timeInfo.tm_mday;
		info->Month = timeInfo.tm_mon + 1;
		info->Year = timeInfo.tm_year + 1900;

		std::snprintf(info->Filename, sizeof(info->Filename), "%s/%04d-%02d-%02d.txt",
			info->Directory, info->Year, info->Month, info->Day);

		if (info->File.is_open() != false)
		{
			info->File.close();
		}

		info->File.open(info->Filename, std::ios::out | std::ios::app);
		return (info->File.is_open() != false);
	}
}

CLog::CLog() // OK
{
	this->m_count = 0;
}

CLog::~CLog() // OK
{
	for (int n = 0; n < this->m_count; n++)
	{
		if (this->m_LogInfo[n].File.is_open() != false)
		{
			this->m_LogInfo[n].File.close();
		}
	}
}

void CLog::AddLog(BOOL active, char* directory) // OK
{
	if (this->m_count < 0 || this->m_count >= MAX_LOG)
	{
		return;
	}

	LOG_INFO* lpInfo = &this->m_LogInfo[this->m_count++];
	lpInfo->Active = active;
	strcpy_s(lpInfo->Directory, directory);

	if (lpInfo->Active != 0 && OpenDailyLogFile(lpInfo) == false)
	{
		lpInfo->Active = 0;
	}
}

void CLog::Output(eLogType type, char* text, ...) // OK
{
	if (type < 0 || type >= this->m_count)
	{
		return;
	}

	LOG_INFO* lpInfo = &this->m_LogInfo[type];
	if (lpInfo->Active == 0)
	{
		return;
	}

	std::time_t rawTime = std::time(nullptr);
	std::tm timeInfo;
	if (localtime_s(&timeInfo, &rawTime) != 0)
	{
		return;
	}

	if (timeInfo.tm_mday != lpInfo->Day || (timeInfo.tm_mon + 1) != lpInfo->Month || (timeInfo.tm_year + 1900) != lpInfo->Year)
	{
		if (OpenDailyLogFile(lpInfo) == false)
		{
			lpInfo->Active = 0;
			return;
		}
	}

	char temp[1024] = {0};

	va_list arg;
	va_start(arg, text);
	vsprintf_s(temp, text, arg);
	va_end(arg);

	char buff[1024] = {0};
	std::snprintf(buff, sizeof(buff), "%02d:%02d:%02d %s\n", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, temp);

	lpInfo->File << buff;
	lpInfo->File.flush();
}
