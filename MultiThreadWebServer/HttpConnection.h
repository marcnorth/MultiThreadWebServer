#pragma once

#include <WS2tcpip.h>
#include <filesystem>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpConnection {

private:
	static const int MAX_REQUEST_LENGTH = 8192;
	SOCKET socket;
	std::filesystem::path wwwRoot;
	std::string requestStr;
	std::map<std::string, std::string> headers;

public:
	explicit HttpConnection(SOCKET socket, std::filesystem::path wwwRoot);
	~HttpConnection();
	HttpConnection(const HttpConnection&) = delete;
	void operator=(const HttpConnection&) = delete;
	HttpConnection(HttpConnection&& other) noexcept;
	void process();

private:
	std::string getRequestStr();
	HttpRequest parseRequest();
	std::string getRequestLine();
	std::string getHost();
	std::map<std::string, std::string> getRequestHeaders();
	void checkRequestLineVersion(const std::string& requestLine);
	HttpRequest::Method getMethodFromRequestLine(const std::string& requestLine);
	std::string getPathFromRequestLine(const std::string& requestLine);
	std::filesystem::path requestFilePath(const HttpRequest& request);
	std::string getFileContents(const std::filesystem::path& filePath);
	HttpResponse createResponse(const HttpRequest& request);
	void sendResponse(const HttpResponse& response);

public:
	static void processConnections(std::queue<HttpConnection>& connections, const bool& running, std::mutex&, std::condition_variable&);
};