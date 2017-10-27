#include "Constant.h"

#include <cstring>
#include <sstream>
#include <iomanip>
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
	Clear();
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

void Constant::Clear()
{
	if( type == CT_String && string )
		delete string;
	else if( type == CT_CodeObject && codeObject )
		delete codeObject;
	
	type = CT_Nil;
	integer = 0;
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

char* Constant::ReadConstant(char* memorySource)
{
	Clear();
	
	switch( reinterpret_cast<Constant*>(memorySource)->type )
	{
	case CT_Nil:
	case CT_Bool:
	case CT_Integer:
	case CT_Float:
		memcpy(this, memorySource, sizeof(Constant));
		return memorySource + sizeof(Constant);
	
	case CT_String:
	{
		memcpy(&type, memorySource, sizeof(Constant::Type));
		memorySource += sizeof(Constant::Type);
		
		unsigned charsCount = 0;

		memcpy(&charsCount, memorySource, sizeof(unsigned));
		memorySource += sizeof(unsigned);

		unsigned size = charsCount * sizeof(char);
		string = new std::string(memorySource, size);
		memorySource += size;
		
		return memorySource;
	}
		
	case CT_CodeObject:
	{
		memcpy(&type, memorySource, sizeof(Constant::Type));
		memorySource += sizeof(Constant::Type);
		
		unsigned closureSize = 0;
		
		memcpy(&closureSize, memorySource, sizeof(unsigned));
		memorySource += sizeof(unsigned);
		
		unsigned instructionsCount = 0;
		
		memcpy(&instructionsCount, memorySource, sizeof(unsigned));
		memorySource += sizeof(unsigned);
		
		unsigned linesCount = 0;
		
		memcpy(&linesCount, memorySource, sizeof(unsigned));
		memorySource += sizeof(unsigned);
		
		int localsCount = 0;
				
		memcpy(&localsCount, memorySource, sizeof(int));
		memorySource += sizeof(int);
		
		int paramsCount = 0;
		
		memcpy(&paramsCount, memorySource, sizeof(int));
		memorySource += sizeof(int);
				
		codeObject = new CodeObject();
		
		if( closureSize > 0 )
		{
			codeObject->closureMapping.assign((int*)memorySource, (int*)memorySource + closureSize);
			memorySource += closureSize * sizeof(int);
		}
		
		if( instructionsCount > 0 )
		{
			codeObject->instructions.assign((Instruction*)memorySource, (Instruction*)memorySource + instructionsCount);
			memorySource += instructionsCount * sizeof(Instruction);
		}
		
		if( linesCount > 0 )
		{
			codeObject->instructionLines.assign((SourceCodeLine*)memorySource, (SourceCodeLine*)memorySource + linesCount);
			memorySource += linesCount * sizeof(SourceCodeLine);
		}
		
		codeObject->localVariablesCount = localsCount;
		codeObject->namedParametersCount = paramsCount;
		
		return memorySource;
	}
	}
	
	return memorySource;
}

std::string Constant::AsDebugString() const
{
	using namespace std::string_literals;
	
	switch( type )
	{
	case Constant::CT_Nil:
		return "nil\n"s;

	case Constant::CT_Integer:
		return "int "s + std::to_string(integer) + "\n";

	case Constant::CT_Float:
		return "float "s + std::to_string(floatingPoint) + "\n";

	case Constant::CT_Bool:
		return "bool "s + (boolean ? "true\n" : "false\n");

	case Constant::CT_String:
		return "string \""s + *string + "\"\n";
	
	case CT_CodeObject:
	{
		std::stringstream result;
		result << "function - " << codeObject->localVariablesCount << " locals (" << codeObject->namedParametersCount << " parameters)\n";

		unsigned linesIndex = 0;
		unsigned closureSize = codeObject->closureMapping.size();
		unsigned linesSize = codeObject->instructionLines.size();
		unsigned instructionsSize = codeObject->instructions.size();
		
		if( closureSize > 0 )
		{
			result << "           closure created from:\n";
			
			for( unsigned i = 0; i < closureSize; ++i )
			{
				result << "           [" << i << "] ";

				int variableIndex = codeObject->closureMapping[i];
				if( variableIndex >= 0 )
					result << "local boxed " << variableIndex << "\n";
				else
					result << "free variable " << (-variableIndex - 1) << "\n";
			}
		}

		for( unsigned i = 0; i < instructionsSize; ++i )
		{
			Instruction* instruction = &codeObject->instructions[i];

			if( linesIndex < linesSize && int(i) >= codeObject->instructionLines[linesIndex].instructionIndex )
			{
				result << "       " << std::setw(3) << codeObject->instructionLines[linesIndex].line << " " << std::setw(5) << i << " ";
				++linesIndex;
			}
			else
			{
				result << "           " << std::setw(5) << i << " ";
			}

			result << instruction->AsString() << "\n";
		}
		
		return result.str();
	}
	
	default:
		return "Unknown constant type\n";
	}
}

}
