// SocketManagerUdp.cpp: implementation of the CSocketManagerUdp class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SocketManagerUdp.h"
#include "ServerList.h"
#include "Util.h"

CSocketManagerUdp gSocketManagerUdp;

CSocketManagerUdp::CSocketManagerUdp() // OK
{
	this->m_socket = INVALID_SOCKET;
	this->m_running = false;
	this->m_RecvSize = 0;
	this->m_SendSize = 0;
	std::memset(&this->m_SocketAddr, 0, sizeof(this->m_SocketAddr));
}

CSocketManagerUdp::~CSocketManagerUdp() // OK
{
	this->Clean();
}

bool CSocketManagerUdp::Start(WORD port) // OK
{
	if((this->m_socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP)) == INVALID_SOCKET)
	{
		LogAdd(LOG_RED,(char*)"[SocketManagerUdp] socket() failed with error: %d",errno);
		this->Clean();
		return 0;
	}

	this->m_SocketAddr.sin_family = AF_INET;
	this->m_SocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	this->m_SocketAddr.sin_port = htons(port);

	if(bind(this->m_socket,(sockaddr*)&this->m_SocketAddr,sizeof(this->m_SocketAddr)) == SOCKET_ERROR)
	{
		LogAdd(LOG_RED,(char*)"[SocketManagerUdp] bind() failed with error: %d",errno);
		this->Clean();
		return 0;
	}

	this->m_running = true;

	try
	{
		this->m_ServerRecvThread = std::thread(&CSocketManagerUdp::ServerRecvThread, this);
	}
	catch (...)
	{
		LogAdd(LOG_RED,(char*)"[SocketManagerUdp] Failed to create recv thread");
		this->Clean();
		return 0;
	}

	memset(this->m_RecvBuff,0,sizeof(this->m_RecvBuff));
	this->m_RecvSize = 0;
	return 1;
}

bool CSocketManagerUdp::Connect(char* IpAddress,WORD port) // OK
{
	this->m_SocketAddr.sin_family = AF_INET;
	this->m_SocketAddr.sin_port = htons(port);

	if (inet_pton(AF_INET, IpAddress, &this->m_SocketAddr.sin_addr) != 1)
	{
		hostent* host = gethostbyname(IpAddress);
		if(host == 0)
		{
			LogAdd(LOG_RED,(char*)"[SocketManagerUdp] gethostbyname() failed with error: %d",errno);
			return 0;
		}

		std::memcpy(&this->m_SocketAddr.sin_addr.s_addr,*host->h_addr_list,host->h_length);
	}

	memset(this->m_SendBuff,0,sizeof(this->m_SendBuff));
	this->m_SendSize = 0;
	return 1;
}

void CSocketManagerUdp::Clean() // OK
{
	this->m_running = false;

	if(this->m_socket != INVALID_SOCKET)
	{
		shutdown(this->m_socket, SHUT_RDWR);
		closesocket(this->m_socket);
		this->m_socket = INVALID_SOCKET;
	}

	if(this->m_ServerRecvThread.joinable())
	{
		this->m_ServerRecvThread.join();
	}
}

bool CSocketManagerUdp::DataRecv() // OK
{
	if(this->m_RecvSize < 3)
	{
		return 1;
	}

	BYTE* lpMsg = this->m_RecvBuff;

	int count=0,size=0;
	BYTE header,head;

	while(true)
	{
		if(lpMsg[count] == 0xC1)
		{
			header = lpMsg[count];
			size = lpMsg[count+1];
			head = lpMsg[count+2];
		}
		else if(lpMsg[count] == 0xC2)
		{
			header = lpMsg[count];
			size = MAKEWORD(lpMsg[count+2],lpMsg[count+1]);
			head = lpMsg[count+3];
		}
		else
		{
			LogAdd(LOG_RED,(char*)"[SocketManagerUdp] Protocol header error (Header: %x)",lpMsg[count]);
			memset(this->m_RecvBuff,0,sizeof(this->m_RecvBuff));
			this->m_RecvSize = 0;
			return 0;
		}

		if(size < 3 || size > MAX_UDP_PACKET_SIZE)
		{
			LogAdd(LOG_RED,(char*)"[SocketManagerUdp] Protocol size error (Header: %x, Size: %d, Head: %x)",header,size,head);
			memset(this->m_RecvBuff,0,sizeof(this->m_RecvBuff));
			this->m_RecvSize = 0;
			return 0;
		}

		if(size <= this->m_RecvSize)
		{
			gServerList.ServerProtocolCore(head,&lpMsg[count],size);

			count += size;
			this->m_RecvSize -= size;

			if(this->m_RecvSize <= 0)
			{
				break;
			}
		}
		else
		{
			if(count > 0 && this->m_RecvSize > 0 && this->m_RecvSize <= (MAX_UDP_PACKET_SIZE-count))
			{
				memmove(lpMsg,&lpMsg[count],this->m_RecvSize);
			}

			break;
		}
	}

	return 1;
}

bool CSocketManagerUdp::DataSend(BYTE* lpMsg,int size) // OK
{
	if(this->m_socket == INVALID_SOCKET)
	{
		return 0;
	}

	if((this->m_SendSize+size) > MAX_UDP_PACKET_SIZE)
	{
		LogAdd(LOG_RED,(char*)"[SocketManagerUdp] Max msg size (Size: %d)",size);
		memset(this->m_SendBuff,0,sizeof(this->m_SendBuff));
		this->m_SendSize = 0;
		return 0;
	}

	memcpy(&this->m_SendBuff[this->m_SendSize],lpMsg,size);
	this->m_SendSize += size;

	int result = sendto(this->m_socket,(char*)this->m_SendBuff,this->m_SendSize,0,(sockaddr*)&this->m_SocketAddr,sizeof(this->m_SocketAddr));

	if(result == SOCKET_ERROR)
	{
		LogAdd(LOG_RED,(char*)"[SocketManagerUdp] sendto() failed with error: %d",errno);
		memset(this->m_SendBuff,0,sizeof(this->m_SendBuff));
		this->m_SendSize = 0;
		return 0;
	}

	this->m_SendSize -= result;
	memmove(this->m_SendBuff,&this->m_SendBuff[result],this->m_SendSize);
	return 1;
}

void CSocketManagerUdp::ServerRecvThread(CSocketManagerUdp* lpSocketManagerUdp) // OK
{
	while(lpSocketManagerUdp->m_running != false)
	{
		SOCKADDR_IN socketAddr = {};
		socklen_t socketAddrSize = sizeof(socketAddr);

		int result = recvfrom(lpSocketManagerUdp->m_socket,(char*)&lpSocketManagerUdp->m_RecvBuff[lpSocketManagerUdp->m_RecvSize],(MAX_UDP_PACKET_SIZE-lpSocketManagerUdp->m_RecvSize),0,(sockaddr*)&socketAddr,&socketAddrSize);

		if(result == SOCKET_ERROR)
		{
			if (lpSocketManagerUdp->m_running == false)
			{
				break;
			}

			LogAdd(LOG_RED,(char*)"[SocketManagerUdp] recvfrom() failed with error: %d",errno);
			memset(lpSocketManagerUdp->m_RecvBuff,0,sizeof(lpSocketManagerUdp->m_RecvBuff));
			lpSocketManagerUdp->m_RecvSize = 0;
			continue;
		}

		lpSocketManagerUdp->m_RecvSize += result;
		lpSocketManagerUdp->DataRecv();
	}
}
