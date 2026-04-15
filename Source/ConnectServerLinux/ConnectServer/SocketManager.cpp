// SocketManager.cpp: implementation of the CSocketManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SocketManager.h"
#include "ClientManager.h"
#include "ConnectServerProtocol.h"
#include "IpManager.h"
#include "Protect.h"
#include "Util.h"

CSocketManager gSocketManager;

CSocketManager::CSocketManager() // OK
{
	this->m_listen = INVALID_SOCKET;
	this->m_port = 0;
	this->m_running = false;
}

CSocketManager::~CSocketManager() // OK
{
	this->Clean();
}

bool CSocketManager::Start(WORD port) // OK
{
	PROTECT_START;

	this->m_port = port;
	this->m_running = true;

	if (this->CreateListenSocket() == 0)
	{
		this->Clean();
		return 0;
	}

	if (this->CreateAcceptThread() == 0)
	{
		this->Clean();
		return 0;
	}

	if (this->CreateWorkerThread() == 0)
	{
		this->Clean();
		return 0;
	}

	if (this->CreateServerQueue() == 0)
	{
		this->Clean();
		return 0;
	}

	PROTECT_FINAL;

	LogAdd(LOG_BLACK, (char*)"[SocketManager] Server started at port [%d]", this->m_port);
	return 1;
}

void CSocketManager::Clean() // OK
{
	if (this->m_running.exchange(false) == false)
	{
		return;
	}

	if (this->m_listen != INVALID_SOCKET)
	{
		shutdown(this->m_listen, SHUT_RDWR);
		closesocket(this->m_listen);
		this->m_listen = INVALID_SOCKET;
	}

	this->m_ServerQueueCondition.notify_all();

	if (this->m_ServerAcceptThread.joinable())
	{
		this->m_ServerAcceptThread.join();
	}

	if (this->m_ServerWorkerThread.joinable())
	{
		this->m_ServerWorkerThread.join();
	}

	if (this->m_ServerQueueThread.joinable())
	{
		this->m_ServerQueueThread.join();
	}

	for (int n = 0; n < MAX_CLIENT; n++)
	{
		this->Disconnect(n);
	}

	this->m_ServerQueue.ClearQueue();
}

bool CSocketManager::CreateListenSocket() // OK
{
	if ((this->m_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] socket() failed with error: %d", errno);
		return 0;
	}

	int reuseAddress = 1;
	setsockopt(this->m_listen, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuseAddress), sizeof(reuseAddress));

#ifdef SOCKET_LAUNCHER
	int sendBufSize = MAX_MAIN_PACKET_SIZE * 2;
	setsockopt(this->m_listen, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&sendBufSize), sizeof(sendBufSize));
#endif

	SOCKADDR_IN socketAddr = {};
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	socketAddr.sin_port = htons(this->m_port);

	if (bind(this->m_listen, reinterpret_cast<sockaddr*>(&socketAddr), sizeof(socketAddr)) == SOCKET_ERROR)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] bind() failed with error: %d", errno);
		return 0;
	}

	if (listen(this->m_listen, SOMAXCONN) == SOCKET_ERROR)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] listen() failed with error: %d", errno);
		return 0;
	}

	return 1;
}

bool CSocketManager::CreateAcceptThread() // OK
{
	try
	{
		this->m_ServerAcceptThread = std::thread(&CSocketManager::ServerAcceptThread, this);
		return 1;
	}
	catch (...)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] Failed to create accept thread");
		return 0;
	}
}

bool CSocketManager::CreateWorkerThread() // OK
{
	try
	{
		this->m_ServerWorkerThread = std::thread(&CSocketManager::ServerWorkerThread, this);
		return 1;
	}
	catch (...)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] Failed to create worker thread");
		return 0;
	}
}

bool CSocketManager::CreateServerQueue() // OK
{
	try
	{
		this->m_ServerQueueThread = std::thread(&CSocketManager::ServerQueueThread, this);
		return 1;
	}
	catch (...)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] Failed to create queue thread");
		return 0;
	}
}

bool CSocketManager::DataRecv(int index, IO_MAIN_BUFFER* lpIoBuffer) // OK
{
	if (lpIoBuffer->size < 3)
	{
		return 1;
	}

	BYTE* lpMsg = lpIoBuffer->buff;
	int count = 0;
	int size = 0;
	BYTE header;
	BYTE head;

	while (true)
	{
		if (lpMsg[count] == 0xC1)
		{
			header = lpMsg[count];
			size = lpMsg[count + 1];
			head = lpMsg[count + 2];
		}
		else if (lpMsg[count] == 0xC2)
		{
			header = lpMsg[count];
			size = MAKEWORD(lpMsg[count + 2], lpMsg[count + 1]);
			head = lpMsg[count + 3];
		}
		else
		{
			LogAdd(LOG_RED, (char*)"[SocketManager] Protocol header error (Index: %d, Header: %x)", index, lpMsg[count]);
			return 0;
		}

		if (size < 3 || size > MAX_MAIN_PACKET_SIZE)
		{
			LogAdd(LOG_RED, (char*)"[SocketManager] Protocol size error (Index: %d, Header: %x, Size: %d, Head: %x)", index, header, size, head);
			return 0;
		}

		if (size <= lpIoBuffer->size)
		{
			QUEUE_INFO queueInfo = {};
			queueInfo.index = index;
			queueInfo.head = head;
			std::memcpy(queueInfo.buff, &lpMsg[count], size);
			queueInfo.size = size;

			if (this->m_ServerQueue.AddToQueue(&queueInfo) != 0)
			{
				this->m_ServerQueueCondition.notify_one();
			}

			count += size;
			lpIoBuffer->size -= size;

			if (lpIoBuffer->size <= 0)
			{
				break;
			}
		}
		else
		{
			if (count > 0 && lpIoBuffer->size > 0 && lpIoBuffer->size <= (MAX_MAIN_PACKET_SIZE - count))
			{
				std::memmove(lpMsg, &lpMsg[count], lpIoBuffer->size);
			}

			break;
		}
	}

	return 1;
}

bool CSocketManager::SendBuffer(SOCKET socket, const BYTE* buffer, int size)
{
	int sent = 0;

	while (sent < size)
	{
		const int result = send(socket, reinterpret_cast<const char*>(buffer + sent), size - sent, 0);
		if (result > 0)
		{
			sent += result;
			continue;
		}

		if (result == 0)
		{
			return false;
		}

		if (errno == EINTR)
		{
			continue;
		}

		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			pollfd descriptor = {socket, POLLOUT, 0};
			if (poll(&descriptor, 1, 5000) <= 0)
			{
				return false;
			}
			continue;
		}

		return false;
	}

	return true;
}

bool CSocketManager::DataSend(int index, BYTE* lpMsg, int size) // OK
{
	SOCKET socket = INVALID_SOCKET;

	this->m_critical.lock();

	if (CLIENT_RANGE(index) == 0)
	{
		this->m_critical.unlock();
		return 0;
	}

	CClientManager* lpClientManager = &gClientManager[index];
	if (lpClientManager->CheckState() == 0)
	{
		this->m_critical.unlock();
		return 0;
	}

	if (size > MAX_MAIN_PACKET_SIZE)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] Max msg size (Index: %d, Size: %d)", index, size);
		this->m_critical.unlock();
		return 0;
	}

	socket = lpClientManager->m_socket;
	this->m_critical.unlock();

	if (this->SendBuffer(socket, lpMsg, size) == false)
	{
		LogAdd(LOG_RED, (char*)"[SocketManager] send() failed with error: %d", errno);
		this->Disconnect(index);
		return 0;
	}

	return 1;
}

void CSocketManager::Disconnect(int index) // OK
{
	SOCKET socket = INVALID_SOCKET;

	this->m_critical.lock();

	if (CLIENT_RANGE(index) == 0)
	{
		this->m_critical.unlock();
		return;
	}

	CClientManager* lpClientManager = &gClientManager[index];
	if (lpClientManager->CheckState() == 0)
	{
		this->m_critical.unlock();
		return;
	}

	socket = lpClientManager->m_socket;
	lpClientManager->DelClient();

	this->m_critical.unlock();

	if (socket != INVALID_SOCKET)
	{
		shutdown(socket, SHUT_RDWR);
		closesocket(socket);
	}
}

bool CSocketManager::ReceiveClientData(int index)
{
	bool shouldDisconnect = false;

	this->m_critical.lock();

	if (CLIENT_RANGE(index) == 0 || gClientManager[index].CheckState() == 0)
	{
		this->m_critical.unlock();
		return false;
	}

	CClientManager* lpClientManager = &gClientManager[index];
	IO_RECV_CONTEXT* lpIoContext = lpClientManager->m_IoRecvContext;
	if (lpIoContext == nullptr)
	{
		this->m_critical.unlock();
		return false;
	}

	const int available = MAX_MAIN_PACKET_SIZE - lpIoContext->IoMainBuffer.size;
	if (available <= 0)
	{
		this->m_critical.unlock();
		this->Disconnect(index);
		return false;
	}

	const int result = recv(lpClientManager->m_socket,
		reinterpret_cast<char*>(&lpIoContext->IoMainBuffer.buff[lpIoContext->IoMainBuffer.size]),
		available, 0);

	if (result <= 0)
	{
		if (result == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
		{
			shouldDisconnect = true;
		}

		this->m_critical.unlock();

		if (shouldDisconnect != false)
		{
			this->Disconnect(index);
		}

		return false;
	}

	lpIoContext->IoMainBuffer.size += result;

	if (this->DataRecv(index, &lpIoContext->IoMainBuffer) == 0)
	{
		shouldDisconnect = true;
	}

	this->m_critical.unlock();

	if (shouldDisconnect != false)
	{
		this->Disconnect(index);
	}

	return (shouldDisconnect == false);
}

void CSocketManager::ServerAcceptThread(CSocketManager* lpSocketManager) // OK
{
	while (lpSocketManager->m_running != false)
	{
		SOCKADDR_IN socketAddr = {};
		socklen_t socketAddrSize = sizeof(socketAddr);

		SOCKET socket = accept(lpSocketManager->m_listen, reinterpret_cast<sockaddr*>(&socketAddr), &socketAddrSize);
		if (socket == INVALID_SOCKET)
		{
			if (lpSocketManager->m_running == false)
			{
				break;
			}

			if (errno == EINTR)
			{
				continue;
			}

			SleepMs(50);
			continue;
		}

		char ipAddress[16] = {0};
		if (inet_ntop(AF_INET, &socketAddr.sin_addr, ipAddress, sizeof(ipAddress)) == nullptr)
		{
			std::snprintf(ipAddress, sizeof(ipAddress), "0.0.0.0");
		}

		if (gIpManager.CheckIpAddress(ipAddress) == 0)
		{
			closesocket(socket);
			continue;
		}

		int index = GetFreeClientIndex();
		if (index == -1)
		{
			closesocket(socket);
			continue;
		}

		gClientManager[index].AddClient(index, ipAddress, socket);
	}
}

void CSocketManager::ServerWorkerThread(CSocketManager* lpSocketManager) // OK
{
	while (lpSocketManager->m_running != false)
	{
		std::vector<pollfd> descriptors;
		std::vector<int> indexes;

		lpSocketManager->m_critical.lock();
		for (int n = 0; n < MAX_CLIENT; n++)
		{
			if (gClientManager[n].CheckState() != 0)
			{
				pollfd descriptor = {};
				descriptor.fd = gClientManager[n].m_socket;
				descriptor.events = POLLIN;
				descriptor.revents = 0;
				descriptors.push_back(descriptor);
				indexes.push_back(n);
			}
		}
		lpSocketManager->m_critical.unlock();

		if (descriptors.empty() != false)
		{
			SleepMs(100);
			continue;
		}

		const int result = poll(descriptors.data(), descriptors.size(), 250);
		if (result <= 0)
		{
			continue;
		}

		for (std::size_t n = 0; n < descriptors.size(); n++)
		{
			if ((descriptors[n].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
			{
				lpSocketManager->Disconnect(indexes[n]);
				continue;
			}

			if ((descriptors[n].revents & POLLIN) != 0)
			{
				lpSocketManager->ReceiveClientData(indexes[n]);
			}
		}
	}
}

void CSocketManager::ServerQueueThread(CSocketManager* lpSocketManager) // OK
{
	while (lpSocketManager->m_running != false)
	{
		std::unique_lock<std::mutex> lock(lpSocketManager->m_ServerQueueMutex);
		lpSocketManager->m_ServerQueueCondition.wait(lock, [&]() {
			return lpSocketManager->m_running == false || lpSocketManager->m_ServerQueue.GetQueueSize() > 0;
		});

		if (lpSocketManager->m_running == false && lpSocketManager->m_ServerQueue.GetQueueSize() == 0)
		{
			break;
		}

		lock.unlock();

		QUEUE_INFO queueInfo = {};
		if (lpSocketManager->m_ServerQueue.GetFromQueue(&queueInfo) != 0)
		{
			if (CLIENT_RANGE(queueInfo.index) != 0 && gClientManager[queueInfo.index].CheckState() != 0)
			{
				ConnectServerProtocolCore(queueInfo.index, queueInfo.head, queueInfo.buff, queueInfo.size);
			}
		}
	}
}

DWORD CSocketManager::GetQueueSize() // OK
{
	return this->m_ServerQueue.GetQueueSize();
}

#ifdef SOCKET_LAUNCHER
void CSocketManager::send_queue(int index)
{
	while (true)
	{
		IO_SIDE_BUFFER nextBuffer = {};
		bool hasMessage = false;

		this->m_critical.lock();
		if (CLIENT_RANGE(index) != 0 && gClientManager[index].CheckState() != 0)
		{
			IO_SEND_CONTEXT* lpIoContext = gClientManager[index].m_IoSendContext;
			if (lpIoContext != nullptr && lpIoContext->IoSideBuffer.empty() == false)
			{
				nextBuffer = lpIoContext->IoSideBuffer.front();
				lpIoContext->IoSideBuffer.erase(lpIoContext->IoSideBuffer.begin());
				hasMessage = true;
			}
		}
		this->m_critical.unlock();

		if (hasMessage == false)
		{
			break;
		}

		if (this->DataSend(index, nextBuffer.buff, nextBuffer.size) == 0)
		{
			break;
		}
	}
}

bool CSocketManager::add_message_to_queue(IO_SEND_CONTEXT* lpIoContext, BYTE* lpMsg, int size)
{
	if (lpIoContext == nullptr || size > MAX_SIDE_PACKET_SIZE)
	{
		return false;
	}

	IO_SIDE_BUFFER msg = {};
	msg.size = size;
	std::memcpy(msg.buff, lpMsg, size);
	lpIoContext->IoSideBuffer.push_back(msg);
	return true;
}

bool CSocketManager::accumulated_message(int index, BYTE* lpMsg, int size)
{
	this->m_critical.lock();

	if (CLIENT_RANGE(index) == 0 || gClientManager[index].CheckState() == 0)
	{
		this->m_critical.unlock();
		return 0;
	}

	IO_SEND_CONTEXT* lpIoContext = gClientManager[index].m_IoSendContext;
	const bool result = this->add_message_to_queue(lpIoContext, lpMsg, size);

	this->m_critical.unlock();
	return result;
}
#endif
