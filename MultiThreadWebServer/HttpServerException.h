#pragma once

#include <stdexcept>

class HttpServerException : public std::runtime_error {

public:
	explicit HttpServerException(const std::string& message) : std::runtime_error(message) {}

};