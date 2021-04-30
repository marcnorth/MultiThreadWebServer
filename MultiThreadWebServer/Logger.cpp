#include "Logger.h"

void Logger::log(std::string message) {
	printf(message.c_str());
	printf("\n");
}