#include "Tokens.h"

namespace element
{

const char* TokenAsString(Token token)
{
	switch(token)
	{
	case T_EOF:					return "EOF";
	case T_NewLine:				return "\\n";

	case T_Identifier:			return "identifier";

	case T_Integer:				return "integer";
	case T_Float:				return "float";
	case T_String:				return "string";
	case T_Bool:				return "bool";

	case T_If:					return "if";
	case T_Else:				return "else";
	case T_Elif:				return "elif";
	case T_For:					return "for";
	case T_In:					return "in";
	case T_While:				return "while";

	case T_This:				return "this";
	case T_Nil:					return "nil";

	case T_Return:				return "return";
	case T_Break:				return "break";
	case T_Continue:			return "continue";
	case T_Yield:				return "yield ";

	case T_And:					return "and";
	case T_Or:					return "or";
	case T_Xor:					return "xor";
	case T_Not:					return "not";
	case T_SizeOf:				return "#";

	case T_Underscore:			return "_";

	case T_LeftParent:			return "(";
	case T_RightParent:			return ")";

	case T_LeftBrace:			return "{";
	case T_RightBrace:			return "}";

	case T_LeftBracket:			return "[";
	case T_RightBracket:		return "]";

	case T_Column:				return ":";
	case T_Semicolumn:			return ";";
	case T_Comma:				return ",";
	case T_Dot:					return ".";

	case T_Add:					return "+";
	case T_Subtract:			return "-";
	case T_Divide:				return "/";
	case T_Multiply:			return "*";
	case T_Power:				return "^";
	case T_Modulo:				return "%%";
	case T_Concatenate:			return "~";

	case T_AssignAdd:			return "+=";
	case T_AssignSubtract:		return "-=";
	case T_AssignDivide:		return "/=";
	case T_AssignMultiply:		return "*=";
	case T_AssignPower:			return "^=";
	case T_AssignModulo: 		return "%%=";
	case T_AssignConcatenate:	return "~=";

	case T_Assignment:			return "=";

	case T_Equal:				return "==";
	case T_NotEqual:			return "!=";
	case T_Less:				return "<";
	case T_Greater:				return ">";
	case T_LessEqual:			return "<=";
	case T_GreaterEqual:		return ">=";

	case T_Argument:			return "$N";
	case T_ArgumentList:		return "$$";
	case T_Arrow:				return "->";
	case T_ArrayPushBack:		return "<<";
	case T_ArrayPopBack:		return ">>";

	default:
	case T_InvalidToken:		return "InvalidToken";
	}
}

}
