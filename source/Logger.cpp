#include "Logger.h"

#include <stdarg.h>

using namespace std::string_literals;

namespace element
{

void Logger::PushError(const std::string& errorMessage)
{
	mErrorMessages.emplace_back(errorMessage);
}

void Logger::PushError(int line, const std::string& errorMessage)
{
	mErrorMessages.emplace_back("line "s + std::to_string(line) + ": " + errorMessage);
}

void Logger::PushError(const SourceCoords& coords, const std::string& errorMessage)
{
	mErrorMessages.emplace_back("line "s + std::to_string(coords.line) +
								" column "s + std::to_string(coords.column) +
								"\n\t"s + errorMessage);
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
