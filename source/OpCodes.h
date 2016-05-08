#ifndef _OP_CODES_INCLUDED_
#define _OP_CODES_INCLUDED_

#include <vector>

namespace element
{

// TOS == Top Of Stack
// TOS1 == The value beneath TOS
// TOS2 == The value beneath TOS1

enum OpCode : char
{
	OC_Pop,					// pop TOS
	OC_PopN,				// pop A values from the stack
	OC_Rotate2,				// swap TOS and TOS1
	OC_MoveToTOS2,			// copy TOS over TOS2 and pop TOS
	
	// push a value to TOS
	OC_LoadConstant,		// A is the index in the constants vector
	OC_LoadLocal,			// A is the index in the function scope
	OC_LoadGlobal,			// A is the index in the global scope
	OC_LoadNative,			// A is the index in the native functions
	OC_LoadArgument,		// A is the index in the arguments array
	OC_LoadArgsArray,		// load the current frame's arguments array
	OC_LoadThis,			// load the current frame's this object
	
	// store a value from TOS in A
	OC_StoreLocal,			// A is the index in the function scope
	OC_StoreGlobal,			// A is the index in the global scope
	
	// pop a value from TOS and store it in A
	OC_PopStoreLocal,		// A is the index in the function scope
	OC_PopStoreGlobal,		// A is the index in the global scope
	
	// arrays
	OC_MakeArray,			// A is number of elements to be taken from the stack
	OC_LoadElement,			// TOS is the index in the TOS1 array
	OC_StoreElement,		// TOS index, TOS1 array, TOS2 new value
	OC_PopStoreElement,		// TOS index, TOS1 array, TOS2 new value
	OC_ArrayPushBack,
	OC_ArrayPopBack,
	
	// objects
	OC_MakeObject,			// A is number of key-value pairs to be taken from the stack
	OC_MakeEmptyObject,		// make an object with just the proto member value
	OC_LoadHash,			// H is the hash to load on the stack
	OC_LoadMember,			// TOS is the member hash in the TOS1 object
	OC_StoreMember,			// TOS member hash, TOS1 object, TOS2 new value
	OC_PopStoreMember,		// TOS member hash, TOS1 object, TOS2 new value
	
	// generators
	OC_MakeGenerator,		// make a generator object from TOS and replace it at TOS
	OC_GeneratorHasValue,	// call 'has_value' from the TOS object
	OC_GeneratorNextValue,	// call 'next_value' from the TOS object
	
	// closures
	OC_LoadFromBox,			// load the value stored in the box at index A
	OC_StoreToBox,			// A is the index of the box that holds the value
	OC_PopStoreToBox,		// A is the index of the box that holds the value
	OC_MakeClosure,			// A is the number of boxed locals, TOS is the function object

	// move the instruction pointer
	OC_Jump,				// jump to A
	OC_JumpIfFalse,			// jump to A, if TOS is false
	OC_PopJumpIfFalse,		// jump to A, if TOS is false, pop TOS either way
	OC_JumpIfFalseOrPop,	// jump to A, if TOS is false, otherwise pop TOS (and-op)
	OC_JumpIfTrueOrPop,		// jump to A, if TOS is true, otherwise pop TOS (or-op)
	
	OC_FunctionCall,		// function to call and arguments are on stack, A is arguments count
	OC_EndFunction,			// pop function scope, restore previous frame
	
	// binary operations take two arguments from the stack, result in TOS
	OC_Add,
	OC_Subtract,
	OC_Multiply,
	OC_Divide,
	OC_Power,
	OC_Modulo,
	OC_Concatenate,
	OC_Xor,
	
	OC_Equal,
	OC_NotEqual,
	OC_Less,
	OC_Greater,
	OC_LessEqual,
	OC_GreaterEqual,
	
	// unary operations take one argument from the stack, result in TOS
	OC_UnaryPlus,
	OC_UnaryMinus,
	OC_UnaryNot,
	OC_UnaryConcatenate,
	OC_UnarySizeOf,
};


struct Instruction
{
	OpCode opCode;
	
	union
	{
		int A;
		unsigned H;
	};
	
	Instruction(OpCode opCode, int A = 0)
	: opCode(opCode)
	, A(A)
	{}
};


struct AskedVariable
{
	int fromIndex;
	int toIndex;
};


struct CodeSegment
{
	std::vector<AskedVariable> closureMapping;
	std::vector<Instruction> instructions;
	int index;
	int localVariablesCount;
	int namedParametersCount;
	
	std::vector< std::pair<int,int> > instructionLines;
	
	CodeSegment(int index = 0)
	: index(index)
	, localVariablesCount(0)
	, namedParametersCount(0)
	{}
};

}

#endif // _OP_CODES_INCLUDED_