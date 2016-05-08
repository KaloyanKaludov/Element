#include "Symbol.h"

#include <cstring>

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
const unsigned Symbol::HasValueHash = Symbol::Hash("has_value");
const unsigned Symbol::NextValueHash = Symbol::Hash("next_value");


Symbol::Symbol()
: hash(0)
, globalIndex(-1)
{
}

Symbol::Symbol(const std::string& name, unsigned hash, int globalIndex)
: name(name)
, hash(hash)
, globalIndex(globalIndex)
{
}

unsigned Symbol::CalculateSize() const
{
	return	sizeof(unsigned) +			// chars count
			sizeof(char) * name.size() +// chars
			sizeof(unsigned) +			// hash
			sizeof(int);				// global index or -1
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
	
	memcpy(memoryDestination, &globalIndex, sizeof(int));
	memoryDestination += sizeof(int);
	
	return memoryDestination;
}

const char* Symbol::ReadSymbol(const char* memorySource)
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
	
	globalIndex = *((int*)memorySource);
	memorySource += sizeof(int);
	
	return memorySource;
}

}
