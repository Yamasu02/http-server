#pragma once
#include <WS2tcpip.h>
#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>
#pragma comment (lib, "ws2_32.lib")


class ClientSocket
{
public:
	SOCKET sock;

	DWORD BytesSent = 0;
	DWORD SendFlags = 0;
	WSAOVERLAPPED OlSend;

	char DataRecvBuff[4096];
	char DataSendBuff[4096];
	WSABUF RecvBuff = { 4096, DataRecvBuff };
	WSABUF SendBuff = { 4096, DataSendBuff };
	DWORD BytesRecv = 0;
	DWORD RecvFlags = 0;
	WSAOVERLAPPED OlRecv;

	ClientSocket(SOCKET sock)
	{
		this->sock = sock;
		OlSend.hEvent = CreateEvent(0, false, false, "OlSend");
		OlRecv.hEvent = CreateEvent(0, false, false, "OlRecv");
	}

	class SockInfo1
	{
	public:
		SOCKET sock;

	}SockInfo;

};

class  TcpListenerIOCP
{
private:

	const char* IpAddress;
	int	Port;
	std::vector<SOCKET> ListeningSockets;
	static inline std::vector<ClientSocket> ClientSockets{};

	bool running = true;

	WSAEVENT NetworkEvent = WSACreateEvent();

public:

	static inline HANDLE hConsole = NULL;

	enum ConsoleColor
	{
		Blue = 9, Green, Cyan, Red, Pink, Yellow, White
	};

	static void ConsoleLog(const char* str, ConsoleColor Cc)
	{
		SetConsoleTextAttribute(hConsole, Cc);
		std::cout << str << "\n\n";
	}


	void CreateListeningSocket(int port)
	{
		SOCKET ListeningSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (ListeningSocket != INVALID_SOCKET)
		{
			sockaddr_in hint;
			hint.sin_family = AF_INET;
			hint.sin_port = htons(port);
			inet_pton(AF_INET, IpAddress, &hint.sin_addr);

			if (bind(ListeningSocket, (sockaddr*)&hint, sizeof(hint)) != SOCKET_ERROR)
			{
				if (listen(ListeningSocket, SOMAXCONN) != SOCKET_ERROR)
				{
					u_long mode = 1;
					ioctlsocket(ListeningSocket, FIONBIO, &mode);
					ListeningSockets.push_back(ListeningSocket);
					//CreateIoCompletionPort((HANDLE)ListeningSocket, CompletionPort, (DWORD)ListeningSocket, 0);
					return;
				}
				MessageBoxA(NULL, ("listen() failed with error: " + std::to_string(WSAGetLastError())).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
			}
			MessageBoxA(NULL, ("bind() failed with error: " + std::to_string(WSAGetLastError())).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
		}
		MessageBoxA(NULL, ("socket() failed with error: " + std::to_string(WSAGetLastError())).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
	}


	void ListeningLoop()
	{
		while (1)
		{
			for (auto& ListeningSock : ListeningSockets)
			{
				sockaddr_in addr;
				int sz = sizeof(addr);
				SOCKET ClientSock = WSAAccept(ListeningSock, (SOCKADDR*)&addr, &sz, 0, 0);
				if (ClientSock == INVALID_SOCKET)
				{
					switch (WSAGetLastError())
					{
					case WSAEWOULDBLOCK:
						continue;
					}
				}
				else
				{
					ClientSockets.push_back(ClientSocket(ClientSock));
					ClientSockets.front().SendBuff.buf = "";
					ClientSockets.front().SendBuff.len = 0;
					//CreateIoCompletionPort((HANDLE)ClientSock, CompletionPort, (ULONG_PTR)(ClientSockets.data() + ((ClientSockets.size() - 1))), 0);
					//OnClientConnected(ClientSock);
					ConsoleLog(std::string("Client " + std::to_string(ClientSock) + " Connected").c_str(), Green);
				}

			}
			CheckForEvents();
		}
	}

	void CheckForEventCompletion()
	{
		while (1)
		{
			for (int i = 0; i < ClientSockets.size(); i++)
			{
				
				if (WSAGetOverlappedResult(ClientSockets[i].sock, &ClientSockets[i].OlRecv, &ClientSockets[i].BytesRecv, false, &ClientSockets[i].RecvFlags) 
					&& WaitForSingleObject(ClientSockets[i].OlRecv.hEvent,10) == WAIT_OBJECT_0)
				{
					WSAResetEvent(ClientSockets[i].OlRecv.hEvent);
					ConsoleLog(std::string("Bytes read: " + std::to_string(ClientSockets[i].BytesRecv)).c_str(), Green);
					ConsoleLog(std::string("Content: " + std::string(ClientSockets[i].RecvBuff.buf, ClientSockets[i].BytesRecv)).c_str(), Green);
				}
				else
				{
					switch (WSAGetLastError())
					{
					case WSA_IO_INCOMPLETE:
						continue;
					}
				}

				if (WSAGetOverlappedResult(ClientSockets[i].sock, &ClientSockets[i].OlSend, &ClientSockets[i].BytesSent, false, &ClientSockets[i].SendFlags)
					&& WaitForSingleObject(ClientSockets[i].OlSend.hEvent, 10) == WAIT_OBJECT_0)
				{
					WSAResetEvent(ClientSockets[i].OlRecv.hEvent);
					ConsoleLog(std::string("BytesSent: " + std::to_string(ClientSockets[i].BytesSent)).c_str(), Green);
				}
				else
				{
					switch (WSAGetLastError())
					{
					case WSA_IO_INCOMPLETE:
						continue;
					}
				}
			}
		}
	}

	static void Caller(TcpListenerIOCP* t)
	{
		t->CheckForEventCompletion();
	}

	void CheckForEvents()
	{
		for (int i = 0; i < ClientSockets.size(); i++)
		{
			if (!WSAEventSelect(ClientSockets[i].sock, NetworkEvent, FD_READ | FD_WRITE | FD_CLOSE))
			{
				WSANETWORKEVENTS events;
				if (!WSAEnumNetworkEvents(ClientSockets[i].sock, NetworkEvent, &events))
				{
					if (events.lNetworkEvents & FD_READ)
					{
						ConsoleLog("There are bytes to read", Green);
						if (WSARecv(ClientSockets[i].sock, &ClientSockets[i].RecvBuff, 1, &ClientSockets[i].BytesRecv, &ClientSockets[i].RecvFlags, &ClientSockets[i].OlRecv, 0))
						{
							switch (WSAGetLastError())
							{
							case WSA_IO_PENDING:
								ConsoleLog("WSARecv request pending", Yellow);
							default:
								ConsoleLog(std::string("WSARecv failed with error " + std::to_string(WSAGetLastError())).c_str(), Red);
							}
						}
					}
					if (events.lNetworkEvents & FD_WRITE)
					{
						if (ClientSockets[i].SendBuff.len)
						{
							if (!WSASend(ClientSockets[i].sock, &ClientSockets[i].SendBuff, 1, &ClientSockets[i].BytesSent, NULL, &ClientSockets[i].OlSend, 0))
							{
								ClientSockets[i].SendBuff.buf = "";
								ClientSockets[i].SendBuff.len = 0;
							}
							else
								ConsoleLog(std::string("WSASend failed with error: " + std::to_string(WSAGetLastError())).c_str(), Red);
						}
					}
					if (events.lNetworkEvents & FD_CLOSE)
					{
						ConsoleLog(std::string("Client " + std::to_string(ClientSockets[i].sock) + " Disconnected").c_str(), Red);
						closesocket(ClientSockets[i].sock);
						ClientSockets.erase(ClientSockets.begin() + i);
					}
				}
				else
					ConsoleLog(std::string("WSAEnumNetworkEvents failed with error code " + std::to_string(WSAGetLastError())).c_str(), Red);
			}
			else
				ConsoleLog(std::string("WSAEventSelect failed with error code " + std::to_string(WSAGetLastError())).c_str(), Red);
		}
	}




	TcpListenerIOCP(const char* ipAddress, int port) : IpAddress(ipAddress), Port(port)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		WSADATA wsData; WORD ver = MAKEWORD(2, 2);
		int Error = WSAStartup(ver, &wsData);
		if (Error)
		{
			MessageBoxA(NULL, ("WSAStartUp() failed with error: " + std::to_string(Error)).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
		}
		CreateListeningSocket(port);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Caller, this, 0, 0);
		ListeningLoop();
	}


	~TcpListenerIOCP()
	{
		WSACleanup();
	}
};