#include "Constant.h"

#include <cstring>
#include "DataTypes.h"

namespace element
{

Constant::Constant()
: type(CT_Nil)
, integer(0)
{}

Constant::Constant(bool boolean)
: type(CT_Bool)
, boolean(boolean)
{}

Constant::Constant(int integer)
: type(CT_Integer)
, integer(integer)
{}

Constant::Constant(float floatingPoint)
: type(CT_Float)
, floatingPoint(floatingPoint)
{}

Constant::Constant(const std::string& string)
: type(CT_String)
, string(new std::string(string))
{}

Constant::Constant(CodeObject* codeObject)
: type(CT_CodeObject)
, codeObject(codeObject)
{
}

Constant::~Constant()
{
	if( type == CT_String && string )
		delete string;
	else if( type == CT_CodeObject && codeObject )
		delete codeObject;
}

bool Constant::Equals(int i) const
{
	if( type != CT_Integer )
		return false;
	
	return integer == i;
}

bool Constant::Equals(float f) const
{
	if( type != CT_Float )
		return false;
	
	return floatingPoint == f;
}

bool Constant::Equals(const std::string& str) const
{
	if( type != CT_String || !string )
		return false;
	
	return *string == str;
}

unsigned Constant::CalculateSize() const
{
	switch(type)
	{
	case CT_Nil:
	case CT_Bool:
	case CT_Integer:
	case CT_Float:
		return sizeof(Constant);
	
	case CT_String:
		return sizeof(Constant::Type) + sizeof(unsigned) + (string ? string->size() : 0) * sizeof(char);
		
	case CT_CodeObject:
	{
		unsigned closureSize		= codeObject ? codeObject->closureMapping.size() : 0;
		unsigned instructionsCount	= codeObject ? codeObject->instructions.size() : 0;
		unsigned linesCount			= codeObject ? codeObject->instructionLines.size() : 0;
		
		return	sizeof(Constant::Type) + 
				3 * sizeof(unsigned) +
				2 * sizeof(int) +
				closureSize * sizeof(int) +
				instructionsCount * sizeof(Instruction) +
				linesCount * sizeof(SourceCodeLine);
	}
	}
	
	return 0;
}

char* Constant::WriteConstant(char* memoryDestination) const
{
	switch(type)
	{
	case CT_Nil:
	case CT_Bool:
	case CT_Integer:
	case CT_Float:
		memcpy(memoryDestination, this, sizeof(Constant));
		return memoryDestination + sizeof(Constant);
	
	case CT_String:
	{
		Constant::Type type = Constant::CT_String;
		
		memcpy(memoryDestination, &type, sizeof(Constant::Type));
		memoryDestination += sizeof(Constant::Type);
		
		unsigned charsCount = string ? string->size() : 0;

		memcpy(memoryDestination, &charsCount, sizeof(unsigned));
		memoryDestination += sizeof(unsigned);

		if( string && charsCount > 0 )
		{
			unsigned size = charsCount * sizeof(char);
			memcpy(memoryDestination, string->data(), size);
			memoryDestination += size;
		}
		
		return memoryDestination;
	}
		
	case CT_CodeObject:
	{
		Constant::Type type = Constant::CT_CodeObject;
		
		memcpy(memoryDestination, &type, sizeof(Constant::Type));
		memoryDestination += sizeof(Constant::Type);
		
		unsigned closureSize = codeObject ? codeObject->closureMapping.size() : 0;
		
		memcpy(memoryDestination, &closureSize, sizeof(unsigned));
		memoryDestination += sizeof(unsigned);
		
		unsigned instructionsCount = codeObject ? codeObject->instructions.size() : 0;
		
		memcpy(memoryDestination, &instructionsCount, sizeof(unsigned));
		memoryDestination += sizeof(unsigned);
		
		unsigned linesCount = codeObject ? codeObject->instructionLines.size() : 0;
		
		memcpy(memoryDestination, &linesCount, sizeof(unsigned));
		memoryDestination += sizeof(unsigned);
		
		int localsCount = codeObject ? codeObject->localVariablesCount : 0;
				
		memcpy(memoryDestination, &localsCount, sizeof(int));
		memoryDestination += sizeof(int);
		
		int paramsCount = codeObject ? codeObject->namedParametersCount : 0;
		
		memcpy(memoryDestination, &paramsCount, sizeof(int));
		memoryDestination += sizeof(int);
		
		if( codeObject && closureSize > 0 )
		{
			unsigned size = closureSize * sizeof(int);
			memcpy(memoryDestination, codeObject->closureMapping.data(), size);
			memoryDestination += size;
		}
		
		if( codeObject && instructionsCount > 0 )
		{
			unsigned size = instructionsCount * sizeof(Instruction);
			memcpy(memoryDestination, codeObject->instructions.data(), size);
			memoryDestination += size;
		}
		
		if( codeObject && linesCount > 0 )
		{
			unsigned size = linesCount * sizeof(SourceCodeLine);
			memcpy(memoryDestination, codeObject->instructionLines.data(), size);
			memoryDestination += size;
		}
		
		return memoryDestination;
	}
	}
	
	return memoryDestination;
}

}
