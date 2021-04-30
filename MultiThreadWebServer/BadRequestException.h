#pragma once

#include "HttpServerException.h"

class BadRequestException : public HttpServerException {

public:
	explicit BadRequestException(const std::string& message) : HttpServerException(message) {}

};