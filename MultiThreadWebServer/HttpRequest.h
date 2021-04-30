#pragma once

#include <string>
#include <map>

struct HttpRequest {
	enum class Method {
		INVALID,
		GET,
		POST
	};

	Method method = Method::INVALID;
	std::string host;
	std::string path;
	std::map<std::string, std::string> headers;
};