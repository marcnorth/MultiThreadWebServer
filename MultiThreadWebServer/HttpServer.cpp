#include "HttpServer.h"

#include "HttpServerException.h"

#include <ws2tcpip.h>
#include <thread>

HttpServer::HttpServer(const std::string& ipAddress, int portNumber, std::filesystem::path wwwRoot, std::shared_ptr<Logger> logger) :
	ipAddress(ipAddress),
	portNumber(portNumber),
	wwwRoot(wwwRoot),
	logger(logger)
{
	
}

void HttpServer::run() {
	SOCKET listener = createListener();
	createConnectionThreads();
	while (true) {
		logger->log("Waiting for connection");
		SOCKET clientSocket = accept(listener, nullptr, nullptr);
		if (clientSocket != INVALID_SOCKET) {
			logger->log("New connection");
			{
				std::lock_guard<std::mutex> lock(mu);
				connections.push(HttpConnection(clientSocket, wwwRoot));
			}
			condition.notify_one();
		}
	}
	running = false;
	for (std::thread& connectionThread : connectionThreads) {
		connectionThread.join();
	}
}

SOCKET HttpServer::createListener() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw HttpServerException("WSAStartup failed");
	}
	addrinfo* addr = nullptr;
	getAddrInfo(&addr);
	SOCKET listenSocket = INVALID_SOCKET;
	listenSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		freeaddrinfo(addr);
		WSACleanup();
		throw HttpServerException("Error at socket(): " + WSAGetLastError());
	}
	if (bind(listenSocket, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(addr);
		closesocket(listenSocket);
		WSACleanup();
		throw HttpServerException("bind failed with error: " + std::to_string(WSAGetLastError()));
	}
	freeaddrinfo(addr);
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(listenSocket);
		WSACleanup();
		throw HttpServerException("Listen failed with error: " + WSAGetLastError());
	}
	return listenSocket;
}

void HttpServer::getAddrInfo(addrinfo** addr) {
	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, std::to_string(portNumber).c_str(), &hints, addr) != 0) {
		WSACleanup();
		throw HttpServerException("getaddrinfo failed");
	}
}

void HttpServer::createConnectionThreads() {
	if (connectionThreads.size() > 0) {
		return;
	}
	for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
		connectionThreads.push_back(std::thread(
			HttpConnection::processConnections,
			std::ref(connections),
			std::ref(running),
			std::ref(mu),
			std::ref(condition)
		));
	}
}