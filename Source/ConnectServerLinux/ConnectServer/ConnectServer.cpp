#include "stdafx.h"
#include "ConnectServer.h"
#include "MiniDump.h"
#include "Protect.h"
#include "ServerDisplayer.h"
#include "ServerList.h"
#include "SocketManager.h"
#include "SocketManagerUdp.h"
#include "ThemidaSDK.h"
#include "Util.h"
#include "ConnectVersionHex.h"
#include "RuntimeContext.h"

long MaxIpConnection;
long MaxHWIDConnection;
char CustomerName[32];
char CustomerHardwareId[36];

namespace
{
	std::atomic<bool> gServerRunning(false);
	std::thread gTimerThread;
	WORD gBoundTcpPort = 0;
	WORD gBoundUdpPort = 0;

	void TimerLoop()
	{
		DWORD lastServerListTick = GetTickCount();
		DWORD lastDisplayTick = GetTickCount();
		DWORD lastTimeoutTick = GetTickCount();

		while (gServerRunning != false)
		{
			const DWORD currentTick = GetTickCount();

			if ((currentTick - lastServerListTick) >= 1000)
			{
				gServerList.MainProc();
				lastServerListTick = currentTick;
			}

			if ((currentTick - lastDisplayTick) >= 2000)
			{
				gServerDisplayer.Run();
				lastDisplayTick = currentTick;
			}

			if ((currentTick - lastTimeoutTick) >= 5000)
			{
				ConnectServerTimeoutProc();
				lastTimeoutTick = currentTick;
			}

			SleepMs(100);
		}
	}

	void StopRuntime()
	{
		if (gServerRunning.exchange(false) != false)
		{
			if (gTimerThread.joinable())
			{
				gTimerThread.join();
			}
		}

		gSocketManagerUdp.Clean();
		gSocketManager.Clean();
	}

	void HandleSignal(int)
	{
		gServerRunning = false;
	}

	std::string NormalizeCommand(const std::string& command)
	{
		return ToLowerCopy(TrimString(command));
	}

	void PrintStartupInfo()
	{
		const CONNECT_SERVER_RUNTIME_PATHS& paths = GetRuntimePaths();
		std::cout << "[Paths] Base: " << paths.RootDirectory.string() << std::endl;
		std::cout << "[Paths] Data: " << paths.DataDirectory.string() << std::endl;
		std::cout << "[Paths] INI: " << paths.ConnectServerIni.string() << std::endl;
		std::cout << "[Paths] ServerList: " << paths.ServerListDat.string() << std::endl;
	}
}

bool ReloadListCommand()
{
	gServerList.Load((char*)GetServerListDataPath());
	LogAdd(LOG_GREEN, (char*)"[Command] Reload List executado");
	return true;
}

bool ReloadServersCommand()
{
	const WORD previousTcpPort = gBoundTcpPort;
	const WORD previousUdpPort = gBoundUdpPort;

	if (ReloadRuntimeConfig(true) == false)
	{
		LogAdd(LOG_RED, (char*)"[Command] Falha ao recarregar ConnectServer.ini");
		return false;
	}

	LoadServerVersion();
	LogAdd(LOG_GREEN, (char*)"[Command] Reload Servers executado");

	if (GetRuntimeConfig().ConnectServerPortTCP != previousTcpPort || GetRuntimeConfig().ConnectServerPortUDP != previousUdpPort)
	{
		LogAdd(LOG_BLUE, (char*)"[Command] As portas foram alteradas no INI. Reinicie o ConnectServer para aplicar TCP/UDP.");
	}

	return true;
}

int main(int argc, char** argv)
{
	VM_START

	CMiniDump::Start();

	if (InitializeRuntimeContext((argc > 0) ? argv[0] : nullptr) == false)
	{
		std::cerr << "[Error] Nao foi possivel localizar Data/ConnectServer.ini relativo ao executavel." << std::endl;
		return EXIT_FAILURE;
	}

	std::error_code currentPathError;
	std::filesystem::current_path(GetRuntimePaths().RootDirectory, currentPathError);

#if(PROTECT_STATE==1)
#if(CONNECTSERVER_UPDATE>=801)
	gProtect.StartAuth(AUTH_SERVER_TYPE_S8_CONNECT_SERVER);
#elif(CONNECTSERVER_UPDATE>=601)
	gProtect.StartAuth(AUTH_SERVER_TYPE_S6_CONNECT_SERVER);
#elif(CONNECTSERVER_UPDATE>=401)
	gProtect.StartAuth(AUTH_SERVER_TYPE_S4_CONNECT_SERVER);
#else
	gProtect.StartAuth(AUTH_SERVER_TYPE_S2_CONNECT_SERVER);
#endif
#endif

	gServerDisplayer.Init();
	gServerDisplayer.PaintName();
	PrintStartupInfo();

	gBoundTcpPort = GetRuntimeConfig().ConnectServerPortTCP;
	gBoundUdpPort = GetRuntimeConfig().ConnectServerPortUDP;

	if (gSocketManager.Start(gBoundTcpPort) == 0)
	{
		StopRuntime();
		return EXIT_FAILURE;
	}

	if (gSocketManagerUdp.Start(gBoundUdpPort) == 0)
	{
		StopRuntime();
		return EXIT_FAILURE;
	}

	gServerList.Load((char*)GetServerListDataPath());
	LoadServerVersion();

	std::signal(SIGINT, HandleSignal);
	std::signal(SIGTERM, HandleSignal);

	gServerRunning = true;
	gTimerThread = std::thread(TimerLoop);

	std::string command;
	while (gServerRunning != false)
	{
		if (std::getline(std::cin, command) == false)
		{
			break;
		}

		const std::string normalizedCommand = NormalizeCommand(command);
		if (normalizedCommand.empty())
		{
			continue;
		}

		if (normalizedCommand == "reload list")
		{
			ReloadListCommand();
			continue;
		}

		if (normalizedCommand == "reload servers")
		{
			ReloadServersCommand();
			continue;
		}

		if (normalizedCommand == "exit" || normalizedCommand == "quit")
		{
			gServerRunning = false;
			break;
		}

		std::cout << "[Command] Comando invalido. Use: reload list | reload servers | exit" << std::endl;
	}

	StopRuntime();
	CMiniDump::Clean();

	VM_END
	return EXIT_SUCCESS;
}
