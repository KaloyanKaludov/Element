#ifndef _SYMBOL_INCLUDED_
#define _SYMBOL_INCLUDED_

#include <string>

namespace element
{

struct Symbol
{
	static unsigned Hash(const std::string& str);
	static unsigned HashStep(unsigned key);
	
	static const unsigned ProtoHash;
	static const unsigned HasNextHash;
	static const unsigned GetNextHash;
	
	std::string	name;
	unsigned	hash;
	
	Symbol();
	Symbol(const std::string& name, unsigned hash);
	
	unsigned	CalculateSize() const;
	
	char*		WriteSymbol(char* memoryDestination) const;
	char*		ReadSymbol(char* memorySource);
	
	std::string	AsDebugString() const;
};

}

#endif // _SYMBOL_INCLUDED_
