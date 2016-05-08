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
	static const unsigned HasValueHash;
	static const unsigned NextValueHash;
	
	std::string	name;
	unsigned	hash;
	int			globalIndex;
	
	Symbol();
	Symbol(const std::string& name, unsigned hash, int globalIndex = -1);
	
	unsigned CalculateSize() const;
	
	char*		WriteSymbol(char* memoryDestination) const;
	const char*	ReadSymbol(const char* memorySource);
};

}

#endif // _SYMBOL_INCLUDED_