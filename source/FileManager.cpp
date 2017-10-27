#include "FileManager.h"

#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <direct.h>
	#define stat _stat
#else
	#include <unistd.h>
	#include <limits.h>
#endif


bool GetFileExists(const std::string& filename)
{
	struct stat buffer;
	return stat(filename.c_str(), &buffer) == 0;
}

std::string GetExecutableLocation()
{
#ifdef _WIN32
	static char result[MAX_PATH];
	return std::string(result, GetModuleFileName(0, result, MAX_PATH));
#else
	static char result[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
	return std::string(result, count > 0 ? count : 0);
#endif
}

std::string GetCurrentWorkingDirectory()
{
#ifdef _WIN32
	static char result[MAX_PATH];
	return _getcwd(result, MAX_PATH) ? std::string(result) : std::string("");
#else
	static char result[PATH_MAX];
	return getcwd(result, PATH_MAX) ? std::string(result) : std::string("");
#endif
}

#ifdef _WIN32
const char PathDelimiter = '\\';
#else
const char PathDelimiter = '/';
#endif

bool IsPathDelimiter(char c)
{
	return c == '/' || c == '\\';
}

// "a/b.c/d/e.txt" -> "a/b.c/d/"
std::string PathOf(const std::string& pathAndName)
{
	int length = int(pathAndName.size());

	for( int i = length - 1; i >= 0; --i )
		if( IsPathDelimiter(pathAndName[i]) )
			return pathAndName.substr(0, i + 1);
	
	return std::string();
}

// "a/b.c/d/e.txt" -> ".txt"
std::string ExtensionOf(const std::string& filename)
{
	size_t delimiter = filename.find_last_of(PathDelimiter);
	
	if( delimiter == std::string::npos )
		delimiter = 0;
	
	size_t dot = filename.find_last_of('.');
	
	if( dot != std::string::npos && dot >= delimiter )
		return filename.substr(dot);
	
	return std::string();
}

// ""                  -> ""
// "a/b/c.d"           -> "a/b/c.d"
// "dsa//dasd/dsd/sds" -> "dsa/dasd/dsd/sds"
// "C:\\a\\b\\c"       -> "C:/a/b/c"
// "/a/b/../c.d"       -> "/a/c.d"
// "/a/b/b/../../c.d"  -> "/a/c.d"
// "aa/bb/./cc"        -> "aa/bb/cc"
// "./aa/bb/cc/d"      -> "./aa/bb/cc/d"
// "../../c/d"         -> "../../c/d"
std::string NormalizePath(const std::string& path)
{
    struct Range
    {
        int from = 0;
        int to = 0;
		Range(int from, int to) : from(from), to(to) {}
    };
    
    auto isEmpty = [](const Range& r)
    {
        return r.from == r.to;
    };
    
    auto isCurrentDir = [&](const Range& r)
    {
        return (r.to - r.from) == 1 && path[r.from] == '.';
    };
    
    auto isParentDir = [&](const Range& r)
    {
        return (r.to - r.from) == 2 && path[r.from] == '.' && path[r.from + 1] == '.';
    };
    
    std::vector<Range> rangesToCopy;
    
    int size = int(path.size());
    int start = 0;
    int end = 0;
    
    while( start < size )
    {
        end = start + 1;
		
        while( end < size && !IsPathDelimiter(path[end]) )
            ++end;

        Range newRange(start, end);
        
        if( IsPathDelimiter(path[start]) && start != 0 )
            newRange.from++;
        
        if( !isEmpty(newRange) )
        {
            if( !isCurrentDir(newRange) )
            {
                if( isParentDir(newRange) && !rangesToCopy.empty() && !isParentDir(rangesToCopy.back()) )
                    rangesToCopy.pop_back();
                else
                    rangesToCopy.push_back(newRange);
            }
        }
        
        start = end;
    }
    
    std::string result;
    result.reserve(path.size()); // at most as much
    
    for( Range range : rangesToCopy )
        result.append(path, range.from, range.to - range.from).append(&PathDelimiter, 1);
    
    result.pop_back(); // last delimiter
    
    return result;
}

// "a/b/c" + "d/e/f" -> "a/b/c/d/e/f"
std::string ConcatenatePaths(const std::string& lhs, const std::string& rhs)
{
	if( lhs.empty() )
		return rhs;
	
	if( rhs.empty() )
		return lhs;
	
	std::string concatenated;
	
	concatenated.reserve(rhs.size() + lhs.size() + 1);
	
	const std::string& left  = IsPathDelimiter(lhs.back())  ? lhs.substr(0, lhs.size() - 1) : lhs;
	const std::string& right = IsPathDelimiter(rhs.front()) ? rhs.substr(1, rhs.size() - 1) : rhs;
	
	concatenated.append(left);
	concatenated.append(&PathDelimiter, 1);
	concatenated.append(right);
	
	return concatenated;
}


namespace element
{

FileManager::FileManager()
{
	ResetState();
}

FileManager::~FileManager()
{
}

void FileManager::ResetState()
{
	mLocationOfExecutingFile.clear();
	mSearchPaths.clear();
	
	mLocationOfExecutingFile.push_back(GetCurrentWorkingDirectory());
}

void FileManager::AddSearchPath(const std::string& searchPath)
{
	std::string path = NormalizePath(searchPath);
	
	if( std::find(mSearchPaths.begin(), mSearchPaths.end(), path) == mSearchPaths.end() )
		mSearchPaths.push_back(path);
}

void FileManager::ClearSearchPaths()
{
	mSearchPaths.clear();
}

const std::vector<std::string>& FileManager::GetSearchPaths() const
{
	return mSearchPaths;
}

std::string FileManager::GetExecutablePath() const
{
	std::string path = GetExecutableLocation();
	
	auto position = path.find_last_of(PathDelimiter);
	
	if( position != std::string::npos )
		path.resize(position + 1);
	
	return path;
}

std::string FileManager::PushFileToExecute(const std::string& filename)
{
	std::string resolvedFilename = ResolveFile(filename);
	
	if( !resolvedFilename.empty() )
		mLocationOfExecutingFile.push_back(PathOf(resolvedFilename));
	
	return resolvedFilename;
}

void FileManager::PopFileToExecute()
{
	mLocationOfExecutingFile.pop_back();
}

std::string FileManager::ResolveFile(std::string filename) const
{
	if( ExtensionOf(filename).empty() )
		filename.append(".element");
	
	std::string testFilename = ConcatenatePaths(mLocationOfExecutingFile.back(), filename);
	
	if( GetFileExists(testFilename) )
		return NormalizePath(testFilename);
		
	for( const std::string& searchPath : mSearchPaths )
	{
		testFilename = ConcatenatePaths(searchPath, filename);
		
		if( GetFileExists(testFilename) )
			return NormalizePath(testFilename);
	}
	
	return std::string();
}

}
