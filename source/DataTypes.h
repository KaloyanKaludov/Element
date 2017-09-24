#ifndef _DATA_TYPES_INCLUDED_
#define _DATA_TYPES_INCLUDED_

#include <vector>
#include <string>

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


struct CodeObject
{
	std::vector<Instruction> instructions;
	int localVariablesCount;
	int namedParametersCount;

	std::vector<int>			closureMapping;
	std::vector<SourceCodeLine>	instructionLines;

	CodeObject();

	CodeObject(	Instruction* instructions, unsigned instructionsSize,
				SourceCodeLine* lines, unsigned linesSize,
				int localVariablesCount, int namedParametersCount);
};


struct StackFrame
{
	Function*			function		= nullptr;
	const Instruction*	ip				= nullptr;
	const Instruction*	instructions	= nullptr;
	Object*				thisObject		= nullptr;
	std::vector<Value>	variables;
	Array				anonymousParameters;
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

}

#endif // _DATA_TYPES_INCLUDED_
