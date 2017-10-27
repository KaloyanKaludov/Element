#ifndef _DATA_TYPES_INCLUDED_
#define _DATA_TYPES_INCLUDED_

#include <vector>
#include <string>
#include <memory>

#include "GarbageCollected.h"
#include "OpCodes.h"
#include "Value.h"

namespace element
{

struct SourceCodeLine
{
	int line;
	int instructionIndex;
};


struct Module
{
	std::string				filename;
	
	std::vector<Value>		globals;
	Value					result;
	
	std::unique_ptr<char[]>	bytecode;
};


struct CodeObject
{
	std::vector<Instruction>	instructions;
	Module*						module;
	int							localVariablesCount;
	int							namedParametersCount;
	std::vector<int>			closureMapping;
	std::vector<SourceCodeLine>	instructionLines;
	
	CodeObject();
	CodeObject(CodeObject&& o) = default;
	CodeObject(	Instruction* instructions, unsigned instructionsSize,
				SourceCodeLine* lines, unsigned linesSize,
				int localVariablesCount, int namedParametersCount);
};


struct StackFrame
{
	Function*			function		= nullptr;
	const Instruction*	ip				= nullptr;
	const Instruction*	instructions	= nullptr;
	std::vector<Value>*	globals			= nullptr;
	std::vector<Value>	variables;
	Array				anonymousParameters;
	Value				thisObject;
};


struct ExecutionContext
{
	enum State : char
	{
		CRS_NotStarted	= 0,
		CRS_Started		= 1,
		CRS_Finished	= 2,
	};

	State					state	= CRS_NotStarted;
	ExecutionContext*		parent	= nullptr;
	Value					lastObject;
	std::deque<StackFrame>	stackFrames; // this needs to be a deque, because things keep references to it
	std::vector<Value>		stack;
};


std::string BytecodeSymbolsAsDebugString(const char* bytecode);
std::string BytecodeConstantsAsDebugString(const char* bytecode);

}

#endif // _DATA_TYPES_INCLUDED_
