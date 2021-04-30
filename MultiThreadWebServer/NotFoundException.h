#pragma once

#include "HttpServerException.h"

class NotFoundException : public HttpServerException {
	
public:
	explicit NotFoundException(const std::string& message) : HttpServerException(message) {}

};