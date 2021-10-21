#pragma once
#include "TcpListener.h"
#include "HttpRequest.h"
#include "File.h"
#include <fstream>
#include <vector>



class WebServer : public TcpListener,HttpRequest
{
public:

	WebServer(const char* ipAddress, int port) : TcpListener(ipAddress, port) 
	{

	}

protected:

	void HttpRespondToClient(SOCKET ClientSocket, std::string& Content, int ErrorCode)
	{
		std::stringstream Response;
		Response << "HTTP/1.1 " << ErrorCode << " OK\r\n"
			<< "Cache-Control: no-cache, private\r\n"
			<< "Content-Type: text/html\r\n"
			<< "Set-Cookie: Cookie1=xd\r\n"
			<< "Content-Length: " << Content.size() << "\r\n"
			<< "Connection: keep-alive\r\n"
			<< "Keep-Alive: timeout=50, max=1000\r\n" << "\r\n"
			<< Content;

		sendToClient(ClientSocket, Response.str().c_str(), Response.str().size());
	}


	virtual void OnMessageReceived(int ClientSocket, const char* msg, int length)
	{
		ConsoleLog((std::string("Http Request From Client: \n") + std::string(msg)).c_str(), White);

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
			//std::vector<std::string> Elements = GetStrsSepWithChar(ElementList, '&');
			std::vector<std::string> Elements = GetStrsSepWithStr(ElementList, "&");
			for (auto& ElementPair : Elements)
				f1 << ElementPair.c_str() << std::endl;
			f1.close();
			HttpRespondToClient(ClientSocket, std::string("<h1>xdd</h1>"), 200);
		}
	}
};