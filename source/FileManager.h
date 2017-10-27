#ifndef _FILE_MANAGER_INCLUDED_
#define _FILE_MANAGER_INCLUDED_

#include <string>
#include <vector>

namespace element
{

class FileManager
{
public:				FileManager();
					~FileManager();
	
	void			ResetState();
	
	void			AddSearchPath(const std::string& searchPath);
	void			ClearSearchPaths();
	auto			GetSearchPaths() const -> const std::vector<std::string>&;
	
	std::string		GetExecutablePath() const;
	
	// The input file name is relative to the current file being executed.
	// The output file name (if successfully resolved!) is relative to the interpreter.
	// If the file has no extension, it is as if you searched for it with ".element"
	std::string		PushFileToExecute(const std::string& filename);
	void			PopFileToExecute();

protected:
	std::string		ResolveFile(std::string filename) const;
	
private:
	std::vector<std::string> mLocationOfExecutingFile;
	std::vector<std::string> mSearchPaths;
};

}

#endif // _FILE_MANAGER_INCLUDED_
