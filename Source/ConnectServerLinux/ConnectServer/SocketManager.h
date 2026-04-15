// SocketManager.h: interface for the CSocketManager class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "CriticalSection.h"
#include "Queue.h"

#define MAX_MAIN_PACKET_SIZE			8192
#define MAX_SIDE_PACKET_SIZE			8192
#define MAX_SERVER_WORKER_THREAD		1
#define MAX_IO_OPERATION 2
#define IO_RECV 0
#define IO_SEND 1

struct IO_MAIN_BUFFER
{
	BYTE buff[MAX_MAIN_PACKET_SIZE];
	int size;
};

struct IO_SIDE_BUFFER
{
	BYTE buff[MAX_SIDE_PACKET_SIZE];
	int size;
};

struct IO_RECV_CONTEXT
{
	int IoType;
	int IoSize;
	IO_MAIN_BUFFER IoMainBuffer;
};

struct IO_SEND_CONTEXT
{
	int IoType;
	int IoSize;
	IO_MAIN_BUFFER IoMainBuffer;
#ifdef SOCKET_LAUNCHER
	std::vector<IO_SIDE_BUFFER> IoSideBuffer;
#else
	IO_SIDE_BUFFER IoSideBuffer;
#endif
};

class CSocketManager
{
public:
	CSocketManager();
	virtual ~CSocketManager();
	bool Start(WORD port);
	void Clean();
	bool CreateListenSocket();
	bool CreateAcceptThread();
	bool CreateWorkerThread();
	bool CreateServerQueue();
	bool DataRecv(int index,IO_MAIN_BUFFER* lpIoBuffer);
	bool DataSend(int index, BYTE* lpMsg, int size);
	void Disconnect(int index);
	DWORD GetQueueSize();

#ifdef SOCKET_LAUNCHER
	void send_queue(int index);
	bool accumulated_message(int index, BYTE* lpMsg, int size);
	bool add_message_to_queue(IO_SEND_CONTEXT* lpIoContext, BYTE* lpMsg, int size);
#endif
	static void ServerAcceptThread(CSocketManager* lpSocketManager);
	static void ServerWorkerThread(CSocketManager* lpSocketManager);
	static void ServerQueueThread(CSocketManager* lpSocketManager);
private:
	bool SendBuffer(SOCKET socket, const BYTE* buffer, int size);
	bool ReceiveClientData(int index);
	SOCKET m_listen;
	WORD m_port;
	std::atomic<bool> m_running;
	std::thread m_ServerAcceptThread;
	std::thread m_ServerWorkerThread;
	std::thread m_ServerQueueThread;
	CQueue m_ServerQueue;
	std::mutex m_ServerQueueMutex;
	std::condition_variable m_ServerQueueCondition;
	CCriticalSection m_critical;
};

extern CSocketManager gSocketManager;
