// ServerDisplayer.cpp: implementation of the CServerDisplayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ServerDisplayer.h"
#include "Log.h"
#include "Protect.h"
#include "ServerList.h"
#include "SocketManager.h"

CServerDisplayer gServerDisplayer;

CServerDisplayer::CServerDisplayer() // OK
{
	for (int n = 0; n < MAX_LOG_TEXT_LINE; n++)
	{
		std::memset(&this->m_log[n], 0, sizeof(this->m_log[n]));
	}

	strcpy_s(this->m_DisplayerText[0], "STANDBY MODE");
	strcpy_s(this->m_DisplayerText[1], "ACTIVE MODE");
	this->m_lastJoinServerState = false;
	this->m_lastStatusPrintTick = 0;
	this->m_count = 0;
}

CServerDisplayer::~CServerDisplayer() // OK
{
}

void CServerDisplayer::Init() // OK
{
	PROTECT_START
	PROTECT_FINAL
	gLog.AddLog(1, (char*)"LOG");
}

void CServerDisplayer::Run() // OK
{
	this->LogTextPaint();
	this->SetWindowName();
	this->PaintAllInfo();
}

void CServerDisplayer::SetWindowName() // OK
{
	if ((GetTickCount() - this->m_lastStatusPrintTick) < 2000)
	{
		return;
	}

	this->m_lastStatusPrintTick = GetTickCount();

	std::lock_guard<std::mutex> lock(this->m_outputMutex);
	std::cout << "[Status] [" << CONNECTSERVER_VERSION << "] " << CONNECTSERVER_CLIENT
		<< " ConnectServer (QueueSize: " << gSocketManager.GetQueueSize() << ")" << std::endl;
}

void CServerDisplayer::PaintAllInfo() // OK
{
	const bool joinServerState = (gServerList.CheckJoinServerState() != 0);
	if (joinServerState == this->m_lastJoinServerState)
	{
		return;
	}

	this->m_lastJoinServerState = joinServerState;

	std::lock_guard<std::mutex> lock(this->m_outputMutex);
	std::cout << "[Display] " << (joinServerState ? this->m_DisplayerText[1] : this->m_DisplayerText[0]) << std::endl;
}

void CServerDisplayer::PaintName() // OK
{
	std::lock_guard<std::mutex> lock(this->m_outputMutex);
	std::cout << "========================================" << std::endl;
	std::cout << " ConnectServer Linux" << std::endl;
	std::cout << " Comandos: reload list | reload servers | exit" << std::endl;
	std::cout << "========================================" << std::endl;
}

void CServerDisplayer::LogTextPaint() // OK
{
}

void CServerDisplayer::LogAddText(eLogColor color, char* text, int size) // OK
{
	PROTECT_START

	size = ((size >= MAX_LOG_TEXT_SIZE) ? (MAX_LOG_TEXT_SIZE - 1) : size);

	std::memset(&this->m_log[this->m_count].text, 0, sizeof(this->m_log[this->m_count].text));
	std::memcpy(&this->m_log[this->m_count].text, text, size);
	this->m_log[this->m_count].color = color;
	this->m_count = (((++this->m_count) >= MAX_LOG_TEXT_LINE) ? 0 : this->m_count);

	PROTECT_FINAL

	std::lock_guard<std::mutex> lock(this->m_outputMutex);
	std::cout << text << std::endl;
	gLog.Output(LOG_GENERAL, (char*)"%s", &text[9]);
}
