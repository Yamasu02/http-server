#pragma once

#include <WS2tcpip.h>
#include <string>
#include <iostream>
#include <vector>
#pragma comment (lib, "ws2_32.lib")

class ClientSocket
{
	std::string description;
	SOCKET sock;
	//DWORD BytesSent = 0;
	//DWORD BytesRecv = 0;
	//DWORD SendFlags = 0;
	//DWORD RecvFlags = 0;
	//std::string BytesRecv;
	//std::string BytesToSend;

	typedef int(__stdcall* HandleMsg)(const char*);
	HandleMsg HMFuncPtr = nullptr;

	ClientSocket(SOCKET sock,HandleMsg HMFuncPtr = nullptr)
	{
		this->sock = sock;
		if (HMFuncPtr != nullptr)
			this->HMFuncPtr = HMFuncPtr;
	}

	virtual void OnMessageReceived(int clientSocket, const char* msg, int length)
	{
		(*HMFuncPtr)(msg);
		delete[] msg;
	}
};

class TcpListener
{
private:

	const char* IpAddress;
	int	Port;
	fd_set ListeningSockets;
	fd_set ClientSockets;
	//std::vector<ClientSocket> ClientSocks;
	bool running = true;
	timeval TimeOut = { 0,10000 };


protected:

	HANDLE hConsole = NULL;

	enum ConsoleColor
	{
		Blue = 9, Green, Cyan, Red, Pink, Yellow, White
	};

	virtual void ConsoleLog(const char* str,ConsoleColor Cc)
	{
		SetConsoleTextAttribute(hConsole, Cc);
		std::cout << str << "\n\n";
	}

	virtual void OnClientConnected(int ClientSocket)
	{
		ConsoleLog(std::string("Client " + std::to_string(ClientSocket) + " Connected").c_str(), Green);
	}

	virtual void OnClientDisconnected(int ClientSocket)
	{
		ConsoleLog(std::string("Client " + std::to_string(ClientSocket) + " Disconnected").c_str(), Red);
		closesocket(ClientSocket);
		FD_CLR(ClientSocket, &ClientSockets);
	}


public:

	TcpListener(const char* ipAddress, int port) : IpAddress(ipAddress), Port(port)
	{
		hConsole =  GetStdHandle(STD_OUTPUT_HANDLE);
		WSADATA wsData; WORD ver = MAKEWORD(2, 2);
		int Error = WSAStartup(ver, &wsData);
		if (Error)
		{
			MessageBoxA(NULL, ("WSAStartUp() failed with error: " + std::to_string(Error)).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
		}
		FD_ZERO(&ListeningSockets); FD_ZERO(&ClientSockets);
		CreateListeningSocket(port);	
	}

	~TcpListener()
	{
		WSACleanup();
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
					//FD_ZERO(&ListeningSockets);
					FD_SET(ListeningSocket, &ListeningSockets);
					return;
				}
				MessageBoxA(NULL, ("listen() failed with error: " + std::to_string(WSAGetLastError())).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
			}
			MessageBoxA(NULL, ("bind() failed with error: " + std::to_string(WSAGetLastError())).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
		}
		MessageBoxA(NULL, ("socket() failed with error: " + std::to_string(WSAGetLastError())).c_str(), "WinSock Error!", MB_OK | MB_ICONEXCLAMATION); exit(-1);
	}

	void CheckListeningSockets()
	{
		fd_set ReadableSockets = ListeningSockets;
		int socketCount = select(0, &ReadableSockets, nullptr, nullptr, &TimeOut);
		for (int i = 0; i < socketCount; i++)
		{
			SOCKET client = accept(ReadableSockets.fd_array[i], nullptr, nullptr);
			//ClientSocks.push_back(ClientSocket(client));
			FD_SET(client, &ClientSockets);
			OnClientConnected(client);
		}
	}

	void CheckClientSockets()
	{
		fd_set ReadableSockets = ClientSockets;
		int socketCount = select(0, &ReadableSockets, nullptr, nullptr, &TimeOut);
		for (int i = 0; i < socketCount; i++)
		{
			SOCKET CurrClientSock = ReadableSockets.fd_array[i];
			int BytesToRecv;
			int BytesRecv = recv(CurrClientSock,(char*)&BytesToRecv, 4, 0);
			if (BytesRecv == 4)
			{
				char* buf = new char[BytesToRecv]; ZeroMemory(buf, BytesToRecv);
				BytesRecv = recv(CurrClientSock, buf, BytesToRecv, 0);
				int BytesLeft = BytesToRecv - BytesRecv;;
				while (!(BytesRecv == BytesToRecv))
				{
					BytesRecv = recv(CurrClientSock, buf, BytesToRecv - BytesLeft, 0);
					BytesLeft -= BytesRecv;
				}
				//OnMessageReceived(CurrClientSock, buf, BytesRecv);
			}
			else if (BytesRecv == 0)
					OnClientDisconnected(CurrClientSock);
			else if (BytesRecv < 0)
					ConsoleLog(std::string("Client Socket: " + std::to_string(CurrClientSock) + " threw error no " + std::to_string(WSAGetLastError())).c_str(), Red);				
		}	
	}

	void Run()
	{
		while (running)
		{
			CheckListeningSockets();
			CheckClientSockets();
		}
	}
};