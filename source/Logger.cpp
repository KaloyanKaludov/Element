#include "Logger.h"

#include <stdarg.h>

namespace element
{

void Logger::PushError(int line, const char* format, ...)
{
	char buffer[256];
	int charsWritten = sprintf(buffer, "Line %d: ", line);
	
	va_list args;
	va_start(args, format);
	charsWritten += vsprintf(buffer + charsWritten, format, args);
	va_end(args);
	
	buffer[charsWritten++] = '\n';
	buffer[charsWritten] = '\0';
	
	mErrorMessages.emplace_back(buffer);
}

void Logger::PushError(const SourceCoords& coords, const char* format, ...)
{
	char buffer[256];
	int charsWritten = sprintf(buffer, "Line %d Column %d\n\t", coords.line, coords.column);
	
	va_list args;
	va_start(args, format);
	charsWritten += vsprintf(buffer + charsWritten, format, args);
	va_end(args);
	
	buffer[charsWritten++] = '\n';
	buffer[charsWritten] = '\0';
	
	mErrorMessages.emplace_back(buffer);
}

bool Logger::HasErrorMessages() const
{
	return ! mErrorMessages.empty();
}

std::string Logger::GetCombinedErrorMessages() const
{
	std::string combined;
	
	for(auto it = mErrorMessages.rbegin(); it != mErrorMessages.rend(); ++it)
		combined += *it + "\n";
	
	return combined;
}

void Logger::ClearErrorMessages()
{
	mErrorMessages.clear();
}

}
