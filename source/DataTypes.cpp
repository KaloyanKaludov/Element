#include "DataTypes.h"

namespace element
{

CodeObject::CodeObject()
: localVariablesCount(0)
, namedParametersCount(0)
{
}

CodeObject::CodeObject(	Instruction* instructions, unsigned instructionsSize, 
						SourceCodeLine* lines, unsigned linesSize,
						int localVariablesCount, 
						int namedParametersCount )
: instructions(instructions, instructions + instructionsSize)
, localVariablesCount(localVariablesCount)
, namedParametersCount(namedParametersCount)
, instructionLines(lines, lines + linesSize)
{
}

}
