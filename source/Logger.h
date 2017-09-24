#ifndef _LOGGER_H_INCLUDED_
#define _LOGGER_H_INCLUDED_

#include <string>
#include <vector>

namespace element
{

struct SourceCoords
{
	int line;
	int column;

	SourceCoords()
	: line(1)
	, column(1)
	{}
};


class Logger
{
public:
	void		PushError(int line, const char* format, ...);
	void		PushError(const SourceCoords& coords, const char* format, ...);
	
	bool		HasErrorMessages() const;
	std::string	GetCombinedErrorMessages() const;
	void		ClearErrorMessages();

private:
	std::vector<std::string> mErrorMessages;
};

}

#endif // _LOGGER_H_INCLUDED_
