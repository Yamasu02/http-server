#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

class HttpRequest
{
public:

	class RequestLine
	{
	public:
		std::string method, path, HttpVersion;
	}RequestLine;
	std::vector<std::string> headers;
	std::string body;

	std::vector<std::string> GetStrsSepWithChar(std::string& str, const char Separator)
	{
		std::vector<std::string> Strs; std::stringstream ss(str); std::string CurStr;
		if (!str.empty())
			while (std::getline(ss, CurStr, Separator))
				Strs.push_back(CurStr);
		return Strs;
	}

	HttpRequest ParseHttpRequest(std::string& Msg)
	{
		Msg.push_back('\n');
		std::vector<std::string> Strs2 = GetStrsSepWithChar(Msg, '\n');
		//RequestLine
		std::vector<std::string> Rl = GetStrsSepWithChar(Strs2[0], ' ');
		//headers
		int index = std::find(Strs2.begin(), Strs2.end(), "\r") - Strs2.begin();//linear memory structure so subtraction of mem addresses can be made
		auto headers = std::vector<std::string>(&Strs2[1], &Strs2[index]);
		//body
		std::string body = Strs2[Strs2.size() - 1];
		return HttpRequest({ {Rl[0],Rl[1],Rl[2]},headers,body });
	}
};