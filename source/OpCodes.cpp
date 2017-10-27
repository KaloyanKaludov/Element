#include "OpCodes.h"

namespace element
{

Instruction::Instruction(OpCode opCode, int A)
: opCode(opCode)
, A(A)
{
}

std::string Instruction::AsString() const
{
	using namespace std::string_literals;
	
	switch(opCode)
	{
	case OpCode::OC_Pop:				return "Pop";
	case OpCode::OC_PopN:				return "PopN              "s + std::to_string(int(A));
	case OpCode::OC_Rotate2:			return "Rotate2";
	case OpCode::OC_MoveToTOS2:			return "MoveToTOS2";
	case OpCode::OC_Duplicate:			return "Duplicate";
	case OpCode::OC_Unpack:				return "Unpack            "s + std::to_string(int(A));

	case OpCode::OC_LoadConstant:		return "LoadConstant      "s + std::to_string(int(A));
	case OpCode::OC_LoadGlobal:			return "LoadGlobal        "s + std::to_string(int(A));
	case OpCode::OC_LoadLocal:			return "LoadLocal         "s + std::to_string(int(A));
	case OpCode::OC_LoadNative:			return "LoadNative        "s + std::to_string(int(A));
	case OpCode::OC_LoadArgument:		return "LoadArgument      "s + std::to_string(int(A));
	case OpCode::OC_LoadArgsArray:		return "LoadArgsArray";
	case OpCode::OC_LoadThis:			return "LoadThis";

	case OpCode::OC_StoreGlobal:		return "StoreGlobal       "s + std::to_string(int(A));
	case OpCode::OC_StoreLocal:			return "StoreLocal        "s + std::to_string(int(A));
	case OpCode::OC_PopStoreGlobal:		return "PopStoreGlobal    "s + std::to_string(int(A));
	case OpCode::OC_PopStoreLocal:		return "PopStoreLocal     "s + std::to_string(int(A));

	case OpCode::OC_MakeArray:			return "MakeArray         "s + std::to_string(int(A));
	case OpCode::OC_LoadElement:		return "LoadElement";
	case OpCode::OC_StoreElement:		return "StoreElement";
	case OpCode::OC_PopStoreElement:	return "PopStoreElement";

	case OpCode::OC_MakeObject:			return "MakeObject        "s + std::to_string(int(A));
	case OpCode::OC_MakeEmptyObject:	return "MakeEmptyObject";
	case OpCode::OC_LoadHash:			return "LoadHash          "s + std::to_string(unsigned(H));
	case OpCode::OC_LoadMember:			return "LoadMember";
	case OpCode::OC_StoreMember:		return "StoreMember";
	case OpCode::OC_PopStoreMember:		return "PopStoreMember";

	case OpCode::OC_MakeIterator:		return "MakeIterator";
	case OpCode::OC_IteratorHasNext:	return "IteratorHasNext";
	case OpCode::OC_IteratorGetNext:	return "IteratorGetNext";

	case OpCode::OC_MakeBox:			return "MakeBox           "s + std::to_string(int(A));
	case OpCode::OC_LoadFromBox:		return "LoadFromBox       "s + std::to_string(int(A));
	case OpCode::OC_StoreToBox:			return "StoreToBox        "s + std::to_string(int(A));
	case OpCode::OC_PopStoreToBox:		return "PopStoreToBox     "s + std::to_string(int(A));
	case OpCode::OC_MakeClosure:		return "MakeClosure";
	case OpCode::OC_LoadFromClosure:	return "LoadFromClosure   "s + std::to_string(int(A));
	case OpCode::OC_StoreToClosure:		return "StoreToClosure    "s + std::to_string(int(A));
	case OpCode::OC_PopStoreToClosure:	return "PopStoreToClosure "s + std::to_string(int(A));

	case OpCode::OC_Jump:				return "Jump              "s + std::to_string(int(A));
	case OpCode::OC_JumpIfFalse:		return "JumpIfFalse       "s + std::to_string(int(A));
	case OpCode::OC_PopJumpIfFalse:		return "PopJumpIfFalse    "s + std::to_string(int(A));
	case OpCode::OC_JumpIfFalseOrPop:	return "JumpIfFalseOrPop  "s + std::to_string(int(A));
	case OpCode::OC_JumpIfTrueOrPop:	return "JumpIfTrueOrPop   "s + std::to_string(int(A));

	case OpCode::OC_FunctionCall:		return "FunctionCall      "s + std::to_string(int(A));
	case OpCode::OC_Yield:				return "Yield";
	case OpCode::OC_EndFunction:		return "EndFunction";

	case OpCode::OC_Add:				return "Add";
	case OpCode::OC_Subtract:			return "Subtract";
	case OpCode::OC_Multiply:			return "Multiply";
	case OpCode::OC_Divide:				return "Divide";
	case OpCode::OC_Modulo:				return "Modulo";
	case OpCode::OC_Power:				return "Power";
	case OpCode::OC_Concatenate:		return "Concatenate";
	case OpCode::OC_ArrayPushBack:		return "ArrayPushBack";
	case OpCode::OC_ArrayPopBack:		return "ArrayPopBack";
	case OpCode::OC_Xor:				return "Xor";

	case OpCode::OC_Equal:				return "Equal";
	case OpCode::OC_NotEqual:			return "NotEqual";
	case OpCode::OC_Less:				return "Less";
	case OpCode::OC_Greater:			return "Greater";
	case OpCode::OC_LessEqual:			return "LessEqual";
	case OpCode::OC_GreaterEqual:		return "GreaterEqual";

	case OpCode::OC_UnaryMinus:			return "UnaryMinus";
	case OpCode::OC_UnaryPlus:			return "UnaryPlus";
	case OpCode::OC_UnaryNot:			return "UnaryNot";
	case OpCode::OC_UnaryConcatenate:	return "UnaryConcatenate";
	case OpCode::OC_UnarySizeOf:		return "UnarySizeOf";

	default: return "Unknown op code "s + std::to_string(int(opCode));
	}
}

}
