// ClientManager.cpp: implementation of the CClientManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClientManager.h"
#include "ConnectServerProtocol.h"
#include "IpManager.h"
#include "CGMHardwareId.h"
#include "Util.h"

CClientManager gClientManager[MAX_CLIENT];

CClientManager::CClientManager() // OK
{
	this->m_index = -1;
	this->m_state = CLIENT_OFFLINE;
	this->m_socket = INVALID_SOCKET;
	this->m_IoRecvContext = 0;
	this->m_IoSendContext = 0;
	this->m_OnlineTime = 0;
	this->m_PacketTime = 0;

	memset(this->m_IpAddr, 0, sizeof(this->m_IpAddr));
	memset(this->m_HardwareId, 0, sizeof(this->m_HardwareId));
}

CClientManager::~CClientManager() // OK
{
}

bool CClientManager::CheckState() // OK
{
	if (CLIENT_RANGE(this->m_index) == 0 || this->m_state == CLIENT_OFFLINE || this->m_socket == INVALID_SOCKET)
	{
		return 0;
	}

	return 1;
}

bool CClientManager::CheckAlloc() // OK
{
	if (this->m_IoRecvContext == 0 || this->m_IoSendContext == 0)
	{
		return 0;
	}

	return 1;
}

bool CClientManager::CheckOnlineTime() // OK
{
	if ((GetTickCount() - this->m_OnlineTime) > MAX_ONLINE_TIME)
	{
		return 0;
	}

	return 1;
}

void CClientManager::AddClient(int index, char* ip, SOCKET socket) // OK
{
	gClientCount = ((this->CheckAlloc() == 0) ? (((++gClientCount) >= MAX_CLIENT) ? 0 : gClientCount) : gClientCount);

	this->m_IoRecvContext = ((this->m_IoRecvContext == 0) ? (new IO_RECV_CONTEXT) : this->m_IoRecvContext);
	this->m_IoSendContext = ((this->m_IoSendContext == 0) ? (new IO_SEND_CONTEXT) : this->m_IoSendContext);

	this->m_IoRecvContext->IoType = IO_RECV;
	this->m_IoRecvContext->IoSize = 0;
	this->m_IoRecvContext->IoMainBuffer.size = 0;
	std::memset(this->m_IoRecvContext->IoMainBuffer.buff, 0, sizeof(this->m_IoRecvContext->IoMainBuffer.buff));

	this->m_IoSendContext->IoType = IO_SEND;
	this->m_IoSendContext->IoSize = 0;
	this->m_IoSendContext->IoMainBuffer.size = 0;
	std::memset(this->m_IoSendContext->IoMainBuffer.buff, 0, sizeof(this->m_IoSendContext->IoMainBuffer.buff));
#ifdef SOCKET_LAUNCHER
	this->m_IoSendContext->IoSideBuffer.clear();
#else
	this->m_IoSendContext->IoSideBuffer.size = 0;
#endif

	this->m_index = index;
	this->m_state = CLIENT_ONLINE;
	strcpy_s(this->m_IpAddr, ip);
	this->m_socket = socket;
	this->m_OnlineTime = GetTickCount();
	this->m_PacketTime = 0;

	gIpManager.InsertIpAddress(this->m_IpAddr);
	CCServerInitSend(this->m_index, 1);
}

void CClientManager::AddClient(char* HardwareId)
{
	strcpy_s(this->m_HardwareId, HardwareId);
	gmHardwareId.InsertHardwareId(this->m_HardwareId);
}

void CClientManager::DelClient() // OK
{
	gIpManager.RemoveIpAddress(this->m_IpAddr);
	gmHardwareId.RemoveHardwareId(this->m_HardwareId);

	this->m_index = -1;
	this->m_state = CLIENT_OFFLINE;

	memset(this->m_IpAddr, 0, sizeof(this->m_IpAddr));
	memset(this->m_HardwareId, 0, sizeof(this->m_HardwareId));

	this->m_socket = INVALID_SOCKET;
	this->m_OnlineTime = GetTickCount();
	this->m_PacketTime = 0;

	if (this->m_IoRecvContext != 0)
	{
		this->m_IoRecvContext->IoMainBuffer.size = 0;
	}

	if (this->m_IoSendContext != 0)
	{
		this->m_IoSendContext->IoMainBuffer.size = 0;
#ifdef SOCKET_LAUNCHER
		this->m_IoSendContext->IoSideBuffer.clear();
#else
		this->m_IoSendContext->IoSideBuffer.size = 0;
#endif
	}
}
