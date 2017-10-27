#include "Symbol.h"

#include <cstring>
#include <sstream>
#include <iomanip>

namespace element
{

unsigned Symbol::Hash(const std::string& str)
{
	// www.cse.yorku.ca/~oz/hash.html
	unsigned hash = 5381;

	for(char c : str)
		hash = hash * 33 ^ c;

	return hash;
}

unsigned Symbol::HashStep(unsigned hash)
{
	return 7 - hash % 7;
}

const unsigned Symbol::ProtoHash = 0;
const unsigned Symbol::HasNextHash = Symbol::Hash("has_next");
const unsigned Symbol::GetNextHash = Symbol::Hash("get_next");


Symbol::Symbol()
: hash(0)
{
}

Symbol::Symbol(const std::string& name, unsigned hash)
: name(name)
, hash(hash)
{
}

unsigned Symbol::CalculateSize() const
{
	return	sizeof(unsigned) +			// chars count
			sizeof(char) * name.size() +// chars
			sizeof(unsigned);			// hash
}

char* Symbol::WriteSymbol(char* memoryDestination) const
{
	unsigned charsCount = name.size();
	
	memcpy(memoryDestination, &charsCount, sizeof(unsigned));
	memoryDestination += sizeof(unsigned);
	
	if( charsCount > 0 )
	{
		unsigned size = charsCount * sizeof(char);
		memcpy(memoryDestination, name.data(), size);
		memoryDestination += size;
	}
	
	memcpy(memoryDestination, &hash, sizeof(unsigned));
	memoryDestination += sizeof(unsigned);
		
	return memoryDestination;
}

char* Symbol::ReadSymbol(char* memorySource)
{
	unsigned charsCount = *((unsigned*)memorySource);
	memorySource += sizeof(unsigned);
	
	if( charsCount > 0 )
	{
		name = std::string(memorySource, charsCount);
		memorySource += charsCount * sizeof(char);
	}
	
	hash = *((unsigned*)memorySource);
	memorySource += sizeof(unsigned);
	
	return memorySource;
}

std::string Symbol::AsDebugString() const
{
	std::stringstream result;
	
	result << std::setw(10) << hash << "    " << name << "\n";
	
	return result.str();
}

}
