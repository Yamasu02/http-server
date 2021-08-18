#pragma once

#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

class TcpListener
{
public:


	TcpListener(const char* ipAddress, int port) : m_ipAddress(ipAddress), m_port(port)
	{
		//MessageBox(0, "Cant initialize WinSock", 0, MB_OK); exit(-1);
		//Initialize WinSock
		WSADATA wsData;
		WORD ver = MAKEWORD(2, 2);
		if (!WSAStartup(ver, &wsData))
		{
			//Create Listening Socket
			m_socket = socket(AF_INET, SOCK_STREAM, 0);
			if (m_socket != INVALID_SOCKET)
			{
				//Bind Ip address & port to listening socket
				sockaddr_in hint;
				hint.sin_family = AF_INET;
				hint.sin_port = htons(m_port);
				inet_pton(AF_INET, m_ipAddress, &hint.sin_addr);

				if (bind(m_socket, (sockaddr*)&hint, sizeof(hint)) != SOCKET_ERROR)
				{
					//Get Socket to listen for incoming connections 
					if (listen(m_socket, SOMAXCONN) != SOCKET_ERROR)
					{
						FD_ZERO(&m_master);
						FD_SET(m_socket, &m_master);
						return;
					}
				}
			}
		}
	}

	int Run()
	{
		/*Select loops through an array of sockets(a copy of the original array) and remove those which dont have sth to read from*/
		bool running = true;
		while (running)
		{
			fd_set copy = m_master;
			int socketCount = select(0, &copy, nullptr, nullptr, nullptr);
			for (int i = 0; i < socketCount; i++)
			{
				SOCKET sock = copy.fd_array[i];
				if (sock == m_socket)
				{
					//Our listening socket got a connection request
					SOCKET client = accept(m_socket, nullptr, nullptr);
					FD_SET(client, &m_master);
					OnClientConnected(client);
				}
				else
				{
					// a client socket has a message
					char buf[4096];
					ZeroMemory(buf, 4096);

					int bytesIn = recv(sock, buf, 4096, 0);
					if (bytesIn <= 0)
					{
						OnClientDisconnected(sock);
						closesocket(sock);
						FD_CLR(sock, &m_master);
					}
					else
					{
						OnMessageReceived(sock, buf, bytesIn);
					}
				}
			}
		}

		FD_CLR(m_socket, &m_master);
		closesocket(m_socket);

		while (m_master.fd_count > 0)
		{
			SOCKET sock = m_master.fd_array[0];
			FD_CLR(sock, &m_master);
			closesocket(sock);
		}

		WSACleanup();
		return 0;
	}

protected:

	virtual void OnClientConnected(int clientSocket)
	{

	}

	virtual void OnClientDisconnected(int clientSocket)
	{

	}

	virtual void OnMessageReceived(int clientSocket, const char* msg, int length)
	{

	}

	void sendToClient(int clientSocket, const char* msg, int length)
	{
		send(clientSocket, msg, length, 0);
	}

private:

	const char*		m_ipAddress;	
	int				m_port;			
	int				m_socket;		
	fd_set			m_master;		// Master file descriptor set
};
