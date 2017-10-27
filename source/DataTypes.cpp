#include "DataTypes.h"

#include <sstream>
#include <iomanip>
#include "Symbol.h"
#include "Constant.h"

namespace element
{

CodeObject::CodeObject()
: module(nullptr)
, localVariablesCount(0)
, namedParametersCount(0)
{
}

CodeObject::CodeObject(	Instruction* instructions, unsigned instructionsSize, 
						SourceCodeLine* lines, unsigned linesSize,
						int localVariablesCount, 
						int namedParametersCount )
: instructions(instructions, instructions + instructionsSize)
, module(nullptr)
, localVariablesCount(localVariablesCount)
, namedParametersCount(namedParametersCount)
, instructionLines(lines, lines + linesSize)
{
}


std::string BytecodeSymbolsAsDebugString(const char* bytecode)
{
	unsigned* p = (unsigned*)bytecode;

	unsigned symbolsSize	= *p;
	++p;
	// skip symbols count
	++p;
	unsigned symbolsOffset	= *p;
	++p;

	char* symbolIt = (char*)p;
	char* symbolsEnd = symbolIt + symbolsSize;

	std::stringstream result;
	result << "symbols size: " << symbolsSize << " bytes\n";
	
	Symbol currentSymbol;
	int symbolIndex = symbolsOffset;
	
	while( symbolIt < symbolsEnd )
	{
		symbolIt = currentSymbol.ReadSymbol(symbolIt);
		
		result << " " << std::setw(3) << symbolIndex++ << "  " << currentSymbol.AsDebugString();
	}
	
	return result.str();
}


std::string BytecodeConstantsAsDebugString(const char* bytecode)
{
	unsigned* p = (unsigned*)bytecode;

	unsigned symbolsSize = *p;
	++p;
	++p; // skip symbols count
	++p; // skip symbols offset

	char* symbolIt = (char*)p;
	char* symbolsEnd = symbolIt + symbolsSize;

	p = (unsigned*)symbolsEnd;

	unsigned constantsSize	= *p;
	++p;
	// skip constants count
	++p;
	unsigned constantsOffset= *p;
	++p;

	char* constantIt = (char*)p;
	char* constantsEnd = constantIt + constantsSize;

	std::stringstream result;
	result << "constants size: " << constantsSize << " bytes\n";
	
	Constant currentConstant;
	int constantIndex = constantsOffset;

	while( constantIt < constantsEnd )
	{
		constantIt = currentConstant.ReadConstant(constantIt);
		
		result << " " << std::setw(3) << constantIndex++ << "  " << currentConstant.AsDebugString();
	}
	
	return result.str();
}

}
