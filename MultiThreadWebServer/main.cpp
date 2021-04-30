#pragma once

#include "HttpServer.h"
#include "HttpServerException.h"
#include "Logger.h"

#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
	try {
		HttpServer server(
			"0.0.0.0",
			8080,
			std::filesystem::path(argv[0]).parent_path().parent_path().append("www"),
			std::make_shared<Logger>()
		);
		server.run();
	} catch (HttpServerException e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}