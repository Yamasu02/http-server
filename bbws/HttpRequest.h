#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <unordered_map>

class HttpRequest
{
public:

	class RequestLine
	{
	public:
		std::string method, path, HttpVersion;
	}RequestLine;
	std::unordered_map<std::string, std::string> HeadersMap;
	std::string body;

	std::vector<std::string> GetStrsSepWithChar(std::string& str, const char Separator)
	{
		std::vector<std::string> Strs; std::stringstream ss(str); std::string CurStr;
		if (!str.empty())
			while (std::getline(ss, CurStr, Separator))
				Strs.push_back(CurStr);
		return Strs;
	}

	std::vector<std::string> GetStrsSepWithStr(std::string& str, std::string Separator)
	{
		str.append(Separator);
		std::vector<std::string> Strs;
		size_t pos1 = 0, pos2 = 0;
		while ((pos2 = str.find(Separator, pos1)) != std::string::npos)
		{
			Strs.push_back(str.substr(pos1, pos2 - pos1));
			pos1 = pos2 + Separator.size();
		}
		return Strs;
	}

	HttpRequest ParseHttpRequest(std::string& Msg)
	{
		// Splitting raw HTTP request to a vector of strs instead of "\r\n" separation between strs
		std::vector<std::string> Strs2 = GetStrsSepWithStr(Msg, "\r\n");
		//First str is the Request-Line as:Method-FileRequested-HttpVersion
		std::vector<std::string> Rl = GetStrsSepWithChar(Strs2[0], ' ');
		/*Headers are the strs between Request line and body
		i split the title from the content for easy header manipulation in an unordered map(for faster look ups)
		e.g "Cookie: "i am a cookie"" becomes as: key= Cookie, value "i am a cookie" */
		std::unordered_map<std::string, std::string> HeadersMap;
		for (int i = 1; i <= Strs2.size() - 2; i++)
		{
			if (!Strs2[i].empty())
			{
				std::vector<std::string> Pair = GetStrsSepWithStr(Strs2[i], ": ");
				HeadersMap.insert(std::make_pair<std::string, std::string>((std::string)Pair[0], (std::string)Pair[1]));
			}			
		}
		//Body is last str in the vector
		std::string body = Strs2[Strs2.size() - 1];
		return HttpRequest({ {Rl[0],Rl[1],Rl[2]},HeadersMap,body });
	}
};