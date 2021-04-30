#include "HttpConnection.h"

#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <thread>

#include "BadRequestException.h"
#include "NotFoundException.h"

HttpConnection::HttpConnection(SOCKET socket, std::filesystem::path wwwRoot) : socket(socket), wwwRoot(wwwRoot) {

}

HttpConnection::~HttpConnection() {
	if (socket != INVALID_SOCKET) {
		closesocket(socket);
	}
}

HttpConnection::HttpConnection(HttpConnection&& other) noexcept : HttpConnection(other.socket, other.wwwRoot) {
	other.socket = INVALID_SOCKET;
}

void HttpConnection::process() {
	HttpRequest request = parseRequest();
	HttpResponse response = createResponse(request);
	sendResponse(response);
}

HttpRequest HttpConnection::parseRequest() {
	HttpRequest request;
	try {
		std::string requestLine = getRequestLine();
		checkRequestLineVersion(requestLine);
		request.method = getMethodFromRequestLine(requestLine);
		request.path = getPathFromRequestLine(requestLine);
		request.host = getHost();
	}
	catch (BadRequestException e) {
		request.method = HttpRequest::Method::INVALID;
	}
	return request;
}

std::string HttpConnection::getRequestLine() {
	return getRequestStr().substr(0, getRequestStr().find("\r\n"));
}

std::string HttpConnection::getHost() {
	std::map<std::string, std::string> headers = getRequestHeaders();
	try {
		std::string host = headers.at("Host");
		return host.substr(0, host.find(':'));
	} catch (std::out_of_range e) {
		throw BadRequestException("Missing host header");
	}
}

std::map<std::string, std::string> HttpConnection::getRequestHeaders() {
	if (headers.size() == 0) {
		std::istringstream ss;
		ss.str(getRequestStr());
		std::string line;
		std::getline(ss, line);
		for (std::string line; std::getline(ss, line); ) {
			if (line.size() == 1) {
				break;
			}
			int pos = line.find(':');
			if (pos == std::string::npos) {
				throw BadRequestException("Invalid header line: '"+line.substr(0, line.size()-1)+"'");
			}
			headers[line.substr(0, pos)] = line.substr(pos + 2, line.size() - (pos + 3));
		}
	}
	return headers;
}

std::string HttpConnection::getRequestStr() {
	if (requestStr.size() == 0) {
		char buf[MAX_REQUEST_LENGTH];
		ZeroMemory(buf, MAX_REQUEST_LENGTH);
		int byteCount = recv(socket, buf, MAX_REQUEST_LENGTH, 0);
		if (byteCount == 0) {
			throw BadRequestException("Request is empty");
		}
		std::istringstream requestStream(std::string(buf, byteCount));
		requestStr = std::string(buf, byteCount);
	}
	return requestStr;
}

void HttpConnection::checkRequestLineVersion(const std::string& requestLine) {
	if (requestLine.find(" HTTP/1.1") != requestLine.size() - 9) {
		throw BadRequestException("Invalid HTTP Version");
	}
}

HttpRequest::Method HttpConnection::getMethodFromRequestLine(const std::string& requestLine) {
	std::string methodName = requestLine.substr(0, requestLine.find(' '));
	if (methodName == "GET") {
		return HttpRequest::Method::GET;
	} else if (methodName == "POST") {
		return HttpRequest::Method::POST;
	} else {
		return HttpRequest::Method::INVALID;
	}
}

std::string HttpConnection::getPathFromRequestLine(const std::string& requestLine) {
	return requestLine.substr(requestLine.find(' ')+1, requestLine.size() - 13);
}

HttpResponse HttpConnection::createResponse(const HttpRequest& request) {
	HttpResponse response;
	if (request.method == HttpRequest::Method::INVALID) {
		response.status = 400;
		response.body = "Invalid request";
	} else {
		try {
			response.status = 200;
			std::filesystem::path filePath = requestFilePath(request);
			response.body = getFileContents(filePath);
		} catch (NotFoundException e) {
			response.status = 404;
			response.body = "File not found";
		} catch (std::exception e) {
			response.status = 500;
			response.body = "";
		}
	}
	return response;
}

std::filesystem::path HttpConnection::requestFilePath(const HttpRequest& request) {
	std::filesystem::path path(wwwRoot);
	path.append(request.host);
	std::stringstream ss(request.path);
	std::string pathPart;
	while (std::getline(ss, pathPart, '/')) {
		if (pathPart.size() > 0 && pathPart != "..") {
			path.append(pathPart);
		}
	}
	return path;
}

std::string HttpConnection::getFileContents(const std::filesystem::path& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw NotFoundException("File not found");
	}
	return std::string(
		(std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>())
	);
}

void HttpConnection::sendResponse(const HttpResponse& response) {
	std::stringstream ss;
	ss << "HTTP/1.1 ";
	if (response.status == 200) {
		ss << "200 OK\r\n";
	}
	else if (response.status == 400) {
		ss << "400 Bad Request\r\n";
	} else if (response.status == 404) {
		ss << "404 Not Found\r\n";
	}
	ss << "\r\n";
	ss << response.body;
	std::string responseStr = ss.str();
	send(socket, responseStr.c_str(), responseStr.size(), 0);
}

void HttpConnection::processConnections(std::queue<HttpConnection>& connections, const bool& running, std::mutex& mu, std::condition_variable& condition) {
	while (running) {
		std::unique_lock<std::mutex> lock(mu);
		condition.wait(lock, [&running, &connections]() { return !running || !connections.empty(); });
		if (!connections.empty()) {
			HttpConnection connection = std::move(connections.front());
			connections.pop();
			lock.unlock();
			connection.process();
		}
	}
}