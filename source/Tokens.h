#ifndef _TOKENS_H_INCLUDED_
#define _TOKENS_H_INCLUDED_

namespace element
{

enum Token : int
{
	T_EOF = 0,
	T_NewLine,

	T_Identifier,		// name
	
	T_Integer,			// 123
	T_Float,			// 123.456
	T_String,			// "abc"
	T_Bool,				// true false
	
	T_If,				// if
	T_Else,				// else
	T_Elif,				// elif
	T_For,				// for
	T_In,				// in
	T_While,			// while
	
	T_This,				// this
	T_Nil,				// nil

	T_Return,			// return
	T_Break,			// break
	T_Continue,			// continue
	
	T_And,				// and
	T_Or,				// or
	T_Xor,				// xor
	T_Not,				// not
	
	T_Underscore,		// _

	T_LeftParent,		// (
	T_RightParent,		// )
	
	T_LeftBrace,		// {
	T_RightBrace,		// }
	
	T_LeftBracket,		// [
	T_RightBracket,		// ]
	
	T_Column,			// :
	T_DoubleColumn,		// ::
	
	T_Semicolumn,		// ;
	T_Comma,			// ,
	T_Dot,				// .
	
	T_Add,				// +
	T_Subtract,			// -
	T_Divide,			// /
	T_Multiply,			// *
	T_Power,			// ^
	T_Modulo,			// %
	T_Concatenate,		// ~
	
	T_AssignAdd,		// +=
	T_AssignSubtract,	// -=
	T_AssignDivide,		// /=
	T_AssignMultiply,	// *=
	T_AssignPower,		// ^=
	T_AssignModulo,		// %=
	T_AssignConcatenate,// ~=
	
	T_Assignment,		// =
	
	T_Equal,			// ==
	T_NotEqual,			// !=
	T_Less,				// <
	T_Greater,			// >
	T_LessEqual,		// <=
	T_GreaterEqual,		// >=
	
	T_Argument,			// $ $1 $2 ...
	T_ArgumentList,		// $$
	T_Arrow,			// ->
	T_ArrayPushBack,	// <<
	T_ArrayPopBack,		// >>
	T_SizeOf,			// #
	
	T_InvalidToken
};

}

#endif // _TOKENS_H_INCLUDED_
