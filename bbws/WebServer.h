#pragma once
#include "TcpListener.h"
#include "HttpRequest.h"
#include "File.h"
#include <fstream>
#include <vector>




class WebServer : public TcpListener,HttpRequest
{
public:

	WebServer(const char* ipAddress, int port) :
		TcpListener(ipAddress, port) 
	{

	}

protected:

	void HttpRespondToClient(SOCKET ClientSocket, std::string& Content, int ErrorCode)
	{
		std::stringstream Response;
		Response << "HTTP/1.1 " << ErrorCode << " OK\r\n"
			<< "Cache-Control: no-cache, private\r\n"
			<< "Content-Type: text/html\r\n"
			<< "Content-Length: " << Content.size() << "\r\n"
			<< "Connection: keep-alive\r\n"
			<< "Keep-Alive: timeout=50, max=1000\r\n" << "\r\n"
			<< Content;

		sendToClient(ClientSocket, Response.str().c_str(), Response.str().size());
	}

	virtual void OnClientConnected(int ClientSocket)
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, 2);
		std::cout << "Client " << ClientSocket << " connected" << std::endl;
		SetConsoleTextAttribute(hConsole, 7);
	}

	virtual void OnClientDisconnected(int ClientSocket)
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "Client " << ClientSocket << " Disconnected" << std::endl;
		SetConsoleTextAttribute(hConsole, 7);
	}

	virtual void OnMessageReceived(int ClientSocket, const char* msg, int length)
	{
		std::cout << "Http Request From Client: \n\n" << msg << std::endl;

		HttpRequest ParsedHttpResponse = ParseHttpRequest(std::string(msg));
		if (ParsedHttpResponse.RequestLine.method == ("GET"))
		{
			std::string htmlFile = ParsedHttpResponse.RequestLine.path;
			if (htmlFile == "/")
				htmlFile = "/index.html";

			Logger HtmlFile(std::string(".\\wwwroot" + htmlFile).c_str());
			std::string content = HtmlFile.ReadAllContent().value();
			if (content.empty())
				content = "<h1>404 Not Found</h1>";

			HttpRespondToClient(ClientSocket, content, 200);
		}
		else if (ParsedHttpResponse.RequestLine.method == ("POST"))
		{
			std::string ElementList = ParsedHttpResponse.body;
			std::fstream f1("test.txt", std::ios::out | std::ios::in | std::ios::trunc);
			std::vector<std::string> Elements = GetStrsSepWithChar(ElementList, '&');
			for (auto& ElementPair : Elements)
				f1 << ElementPair.c_str() << std::endl;
			f1.close();
			HttpRespondToClient(ClientSocket, std::string("<h1>xdd</h1>"), 200);
		}
	}
};