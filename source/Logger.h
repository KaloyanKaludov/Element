#ifndef _LOGGER_H_INCLUDED_
#define _LOGGER_H_INCLUDED_

#include <string>
#include <vector>

namespace element
{

struct SourceCoords
{
	int line	= 1;
	int column	= 1;
};


class Logger
{
public:
	void		PushError(const std::string& errorMessage);
	void		PushError(int line, const std::string& errorMessage);
	void		PushError(const SourceCoords& coords, const std::string& errorMessage);
	
	bool		HasErrorMessages() const;
	std::string	GetCombinedErrorMessages() const;
	void		ClearErrorMessages();

private:
	std::vector<std::string> mErrorMessages;
};

}

#endif // _LOGGER_H_INCLUDED_
