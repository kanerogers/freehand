#include "FreehandException.h"
#include <sstream>

FreehandException::FreehandException(int line, const char* file) noexcept
	:
	line(line),
	file(file)
{
}

const char* FreehandException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl << GetOriginString();
	whatBuffer = oss.str(); // copy the string into our private buffer to avoid use after free
	return whatBuffer.c_str();
}

const char* FreehandException::GetType() const noexcept
{
	return "Freehand Exception";
}

int FreehandException::GetLine() const noexcept
{
	return line;
}

const std::string& FreehandException::GetFile() const noexcept
{
	return file;
}

std::string FreehandException::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << std::endl << "[Line] " << line;
	return oss.str();
}
