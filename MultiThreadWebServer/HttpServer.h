#pragma once

#include <string>
#include <ws2tcpip.h>
#include <memory>
#include <filesystem>
#include <thread>
#include <condition_variable>
#include <queue>

#include "HttpConnection.h"
#include "Logger.h"

class HttpServer {

private:
	std::string ipAddress;
	int portNumber;
	std::filesystem::path wwwRoot;
	std::shared_ptr<Logger> logger;
	std::queue<HttpConnection> connections;
	std::vector<std::thread> connectionThreads;
	bool running = true;
	std::mutex mu;
	std::condition_variable condition;

public:
	HttpServer(const std::string& ipAddress, int port, std::filesystem::path wwwRoot, std::shared_ptr<Logger> logger);
	void run();

private:
	SOCKET createListener();
	void getAddrInfo(addrinfo**);
	void createConnectionThreads();

};