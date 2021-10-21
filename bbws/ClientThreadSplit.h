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
	//DWORD BytesRecv = 0;

	std::string RecvData = "";
	std::string ToSendData = "";

	typedef int(__stdcall* HandleMsg)(const char*);
	HandleMsg HMFuncPtr = nullptr;
	DWORD ThreadId;

	ClientSocket(SOCKET sock,HandleMsg HMFuncPtr = nullptr)
	{
		this->sock = sock;
		if(HMFuncPtr != nullptr)
			this->HMFuncPtr = HMFuncPtr;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Loop, this, 0, &ThreadId);
	}



	static void Loop(ClientSocket* cs)
	{
		WSAEVENT NetworkEvent = WSACreateEvent();
		WSANETWORKEVENTS events;

		while (1)
		{
			if (!WSAEventSelect(cs->sock, NetworkEvent, FD_READ | FD_WRITE | FD_CLOSE))
			{
				if (!WSAEnumNetworkEvents(cs->sock, NetworkEvent, &events))
				{
					if (events.lNetworkEvents & FD_READ)
					{
						cs->RecvData.clear();
						int BytesRecv = 0;
						do
						{
							char buff[4096];
							BytesRecv = recv(cs->sock, buff, 4096, 0);
							if (BytesRecv > 0)
								cs->RecvData.append(buff, BytesRecv);
						} while (BytesRecv > 0);
						//handle recved msg
						if(cs->HMFuncPtr != nullptr)
							(*cs->HMFuncPtr)(cs->RecvData.c_str());
					}
					if ((events.lNetworkEvents & FD_WRITE) && !cs->ToSendData.empty())
					{
						cs->BytesSent = send(cs->sock, cs->ToSendData.c_str(), cs->ToSendData.size(), 0);
						//cs->SendData(it->second.ToSendData.c_str(), it->second.ToSendData.size());
						cs->ToSendData.clear();
					}
					if (events.lNetworkEvents & FD_CLOSE)
					{

					}
				}
			}
		}
	}
};


class  TcpListenerClientMT
{
private:

	const char* IpAddress;
	int	Port;
	std::vector<SOCKET> ListeningSockets;
	static inline std::vector<ClientSocket> ClientSockets{};

	//bool running = true;
	//WSAEVENT NetworkEvent = WSACreateEvent();

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
					//OnClientConnected(ClientSock);
					ConsoleLog(std::string("Client " + std::to_string(ClientSock) + " Connected").c_str(), Green);
				}

			}
			//CheckForEvents();
		}
	}


	//void CheckForEvents()
	//{
	//	for (int i = 0; i < ClientSockets.size(); i++)
	//	{
	//		if (GetAsyncKeyState(VK_BACK))
	//		{
	//			ClientSockets[i].SendBuff.buf = "reeee";
	//			ClientSockets[i].SendBuff.len = 5;
	//		}
	//		if (!WSAEventSelect(ClientSockets[i].sock, NetworkEvent, FD_READ | FD_WRITE | FD_CLOSE))
	//		{
	//			WSANETWORKEVENTS events;
	//			if (!WSAEnumNetworkEvents(ClientSockets[i].sock, NetworkEvent, &events))
	//			{
	//				if (events.lNetworkEvents & FD_READ)
	//				{
	//					ConsoleLog("There are bytes to read", Green);
	//					if (!WSARecv(ClientSockets[i].sock, &ClientSockets[i].RecvBuff, 1, &ClientSockets[i].BytesRecv, &ClientSockets[i].RecvFlags, &ClientSockets[i].OlRecv, 0))
	//					{
	//						ConsoleLog(std::string("WSARecv is ok,bytes read: " + std::to_string(ClientSockets[i].BytesRecv)).c_str(), Green);
	//						if (WSAGetOverlappedResult(ClientSockets[i].sock, &ClientSockets[i].OlRecv, &ClientSockets[i].BytesRecv, true, &ClientSockets[i].RecvFlags))
	//						{
	//							ConsoleLog(std::string("Bytes read: " + std::to_string(ClientSockets[i].BytesRecv)).c_str(), Green);
	//							ConsoleLog(std::string("Content: " + std::string(ClientSockets[i].RecvBuff.buf, ClientSockets[i].BytesRecv)).c_str(), Green);
	//						}
	//					}
	//					else
	//					{
	//						switch (WSAGetLastError())
	//						{
	//						case WSA_IO_PENDING:
	//							ConsoleLog("WSARecv request pending", Red);
	//						default:
	//							ConsoleLog(std::string("WSARecv failed with error " + std::to_string(WSAGetLastError())).c_str(), Red);
	//						}
	//					}
	//				}
	//				if (events.lNetworkEvents & FD_WRITE)
	//				{
	//					if (ClientSockets[i].SendBuff.len)
	//					{
	//						if (!WSASend(ClientSockets[i].sock, &ClientSockets[i].SendBuff, 1, &ClientSockets[i].BytesSent, NULL, &ClientSockets[i].OlSend, 0))
	//						{
	//							if (WSAGetOverlappedResult(ClientSockets[i].sock, &ClientSockets[i].OlSend, &ClientSockets[i].BytesSent, true, &ClientSockets[i].SendFlags))
	//								ConsoleLog(std::string("BytesSent: " + std::to_string(ClientSockets[i].BytesSent)).c_str(), Green);
	//							ClientSockets[i].SendBuff.buf = "";
	//							ClientSockets[i].SendBuff.len = 0;
	//						}
	//						else
	//							ConsoleLog(std::string("WSASend failed with error: " + std::to_string(WSAGetLastError())).c_str(), Red);
	//					}
	//				}
	//				if (events.lNetworkEvents & FD_CLOSE)
	//				{
	//					ConsoleLog(std::string("Client " + std::to_string(ClientSockets[i].sock) + " Disconnected").c_str(), Red);
	//					closesocket(ClientSockets[i].sock);
	//					ClientSockets.erase(ClientSockets.begin() + i);
	//				}
	//			}
	//			else
	//				ConsoleLog(std::string("WSAEnumNetworkEvents failed with error code " + std::to_string(WSAGetLastError())).c_str(), Red);
	//		}
	//		else
	//			ConsoleLog(std::string("WSAEventSelect failed with error code " + std::to_string(WSAGetLastError())).c_str(), Red);
	//	}
	//}


	TcpListenerClientMT(const char* ipAddress, int port) : IpAddress(ipAddress), Port(port)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		WSADATA wsData; WORD ver = MAKEWORD(2, 2);
		int Error = WSAStartup(ver, &wsData);
		if (Error)
		{
			MessageBoxA(NULL, ("WSAStartUp() failed with error: " + std::to_string(Error)).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
		}
		CreateListeningSocket(port);
	}


	~TcpListenerClientMT()
	{
		WSACleanup();
	}
};