#include "Operators.h"

#include <vector>

namespace element
{
/*
right associativity:
	a = (b = c)
left associativity:
	(a + b) + c
*/

const std::vector<OperatorInfo> OperatorsTable =
{
//	operator token				isBinary	isRightAssociative	precedence
	{T_InvalidToken,			false,		false,				0},
	
	{T_Comma,					true,		false,				10},

	{T_Assignment,				true,		true,				20},
	{T_AssignAdd,				true,		true,				20},
	{T_AssignSubtract,			true,		true,				20},
	{T_AssignDivide,			true,		true,				20},
	{T_AssignMultiply,			true,		true,				20},
	{T_AssignPower,				true,		true,				20},
	{T_AssignModulo,			true,		true,				20},
	{T_AssignConcatenate,		true,		true,				20},
	
	{T_ArrayPopBack,			true,		true,				24},
	{T_ArrayPushBack,			true,		true,				25},
	
	{T_Or,						true,		false,				40},

	{T_And,						true,		false,				50},

	{T_Xor,						true,		false,				60},

	{T_Equal,					true,		false,				70},
	{T_NotEqual,				true,		false,				70},

	{T_Less,					true,		false,				80},
	{T_Greater,					true,		false,				80},
	{T_LessEqual,				true,		false,				80},
	{T_GreaterEqual,			true,		false,				80},

	{T_Add,						true,		false,				90},
	{T_Subtract,				true,		false,				90},
	{T_Concatenate,				true,		false,				90},

	{T_Multiply,				true,		false,				100},
	{T_Divide,					true,		false,				100},
	{T_Modulo,					true,		false,				100},

	{T_Power,					true,		false,				110},

	{T_Not,						false,		false,				120},
	{T_Subtract,				false,		false,				120},
	{T_Concatenate,				false,		false,				120},
	{T_SizeOf,					false,		false,				120},
	
	{T_Arrow,					true,		false,				130},
	
	{T_LeftBracket,				false,		false,				150}, // indexing operator
	{T_LeftParent,				false,		false,				150}, // function call
	{T_Column,					true,		false,				150}, // function definition with named arguments
	{T_DoubleColumn,			true,		false,				150}, // function definition without named arguments

	{T_Dot,						true,		false,				150}, // member access
};

const OperatorInfo& GetOperatorInfo(const Token t, bool isBinary)
{
	for( const auto& o : OperatorsTable )
		if( o.token == t && o.isBinary == isBinary )
			return o;
	
	return OperatorsTable.front();
}

}
