#include "Lexer.h"

#include <istream>
#include <string>
#include "Logger.h"

namespace element
{

bool IsSpace(char c)
{
	return c == ' ' || c == '\t';
}

bool IsNewLine(char c)
{
	return c == '\n' || c == '\r';
}

bool IsDigit(char c)
{
	return c >= '0' && c <= '9';
}

bool IsAlpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}


Lexer::Lexer(Logger& logger)
: mInput(nullptr)
, mLogger(logger)
{
	Reset();
}

Lexer::Lexer(std::istream& input, Logger& logger)
: mInput(&input)
, mLogger(logger)
{
	Reset();
}

void Lexer::SetInputStream(std::istream& input)
{
	mInput = &input;
	
	Reset();
}

Token Lexer::GetNextToken()
{
	mLastCharsRead.clear();
	mLastCharsRead.push_back(C);

begining:
	while( IsSpace(C) )
		C = GetNextChar();

	if( IsNewLine(C) )
	{
		C = GetNextChar();
		mCurrentToken = T_NewLine;
		mCurrentCoords.line += 1;
		mCurrentColumn = 1;
		return T_NewLine;
	}

	mCurrentCoords.column = mCurrentColumn;

	if( HandleCommentOrDivision() ) // anything that starts with '/'
	{
		if( mShouldStartOverAgain )
		{
			mShouldStartOverAgain = false;
			goto begining;
		}

		return mCurrentToken;
	}

	if( HandleWords()												|| // identifiers or keywords
		HandleNumbers()												|| // integer or float
		HandleString()												|| // "something..."
		HandleColumns()												|| // : ::
		HandleSubstractOrArrow()									|| // - -= ->
		HandleDollarSign() 											|| // $ $0 $1 $$
		HandleLessOrPush()											|| // < <= <<
		HandleGreaterOrPop()										|| // > >= >>
		HandleSingleChar('(', T_LeftParent) 						||
		HandleSingleChar(')', T_RightParent) 						||
		HandleSingleChar('{', T_LeftBrace) 							||
		HandleSingleChar('}', T_RightBrace) 						||
		HandleSingleChar('[', T_LeftBracket) 						||
		HandleSingleChar(']', T_RightBracket) 						||
		HandleSingleChar(';', T_Semicolumn) 						||
		HandleSingleChar(',', T_Comma) 								||
		HandleSingleChar('.', T_Dot)								||
		HandleSingleChar('#', T_SizeOf)								||
		HandleSingleChar(EOF, T_EOF)								||
		HandleCharAndEqual('+', T_Add, 			T_AssignAdd) 		||
		HandleCharAndEqual('*', T_Multiply, 	T_AssignMultiply) 	||
		HandleCharAndEqual('^', T_Power, 		T_AssignPower) 		||
		HandleCharAndEqual('%', T_Modulo, 		T_AssignModulo) 	||
		HandleCharAndEqual('~', T_Concatenate, 	T_AssignConcatenate)||
		HandleCharAndEqual('=', T_Assignment, 	T_Equal) 			||
		HandleCharAndEqual('!', T_InvalidToken, T_NotEqual) )
	{
		return mCurrentToken;
	}

	mLogger.PushError(mCurrentCoords, std::string("Unrecognized token ") + C);
	return mCurrentToken = T_InvalidToken;;
}

Token Lexer::GetNextToken_IgnoreNewLine()
{
	Token t = GetNextToken();

	while( t == T_NewLine )
		t = GetNextToken();

	return t;
}

Token Lexer::GetCurrentToken() const
{
	return mCurrentToken;
}

// HACK ////////////////////////////////////////////////////////////////////
// This is used when parsing 'if' expressions. We need to check if there will
// be an 'else' clause for the 'if' which will eat the new line, which will
// in turn cause trouble for the 'Parser::ParseExpression' function.
// To prevent this we will "rewind" the lexer input back to its original state
// before eating the new line.
void Lexer::RewindDueToMissingElse()
{
	for( int i = mLastCharsRead.size() - 1; i >= 0; --i )
		mInput->putback( mLastCharsRead[i] );

	C = '\n';

	mCurrentToken = T_NewLine;

	mCurrentCoords.line -= 1;
	mCurrentColumn = 1;
}

const SourceCoords& Lexer::GetCurrentCoords() const
{
	return mCurrentCoords;
}

const std::string& Lexer::GetLastIdentifier() const
{
	return mLastIdentifier;
}

const std::string& Lexer::GetLastString() const
{
	return mLastString;
}

int Lexer::GetLastInteger() const
{
	return mLastInteger;
}

int Lexer::GetLastArgumentIndex() const
{
	return mLastArgumentIndex;
}

float Lexer::GetLastFloat() const
{
	return mLastFloat;
}

bool Lexer::GetLastBool() const
{
	return mLastBool;
}

void Lexer::Reset()
{
	C = ' ';
	
	mCurrentToken		= T_InvalidToken;
	mCurrentCoords		= SourceCoords();
	mCurrentColumn		= 1;
	mLastIdentifier		= "";
	mLastString			= "";
	mLastInteger		= 0;
	mLastArgumentIndex	= 0;
	mLastFloat			= 0.0f;
	mLastBool			= false;
	
	mShouldStartOverAgain = false;
}

char Lexer::GetNextChar()
{
	++mCurrentColumn;

	char ch = mInput->get();

	mLastCharsRead.push_back(ch);

	return ch;
}

bool Lexer::HandleCommentOrDivision()
{
	if( C != '/' ) // this is not a comment or division
		return false;

	C = GetNextChar(); // eat /

	// single line comment ///////////////////////////////////////
	if( C == '/' )
	{
		while( ! IsNewLine(C) )
			C = GetNextChar();
		C = GetNextChar(); // eat the new line

		mCurrentToken = T_NewLine;
		mCurrentCoords.line += 1;
		mCurrentCoords.column = mCurrentColumn = 1;

		return true;
	}

	// multi line comment ////////////////////////////////////////
	if( C == '*' )
	{
		C = GetNextChar(); // eat *

		int nestedLevel = 1; // nested multiline comments
		while(nestedLevel > 0)
		{
			if( C == EOF )
			{
				mLogger.PushError(mCurrentCoords, "Unterminated multiline comment");
				return false;
			}

			while( C != '*' && C != '/' )
			{
				if( IsNewLine(C) )
				{
					mCurrentCoords.line += 1;
					mCurrentCoords.column = mCurrentColumn = 0;
				}

				if( C == EOF )
				{
					mLogger.PushError(mCurrentCoords, "Unterminated multiline comment");
					return false;
				}

				C = GetNextChar();
			}

			if( C == '*' )
			{
				C = GetNextChar(); // eat *

				if( C == '/' )
				{
					C = GetNextChar(); // eat /
					nestedLevel -= 1;
				}
			}
			else // C == '/'
			{
				C = GetNextChar(); // eat /

				if( C == '*' )
				{
					C = GetNextChar(); // eat *
					nestedLevel += 1;
				}
			}
		}

		// get back to searching for next token...
		mShouldStartOverAgain = true;
		return true;
	}

	// dvision with assignment ///////////////////////////////////
	if( C == '=' )
	{
		mCurrentToken = T_AssignDivide;
		C = GetNextChar(); // eat the '='
		return true;
	}

	// just division /////////////////////////////////////////////
	mCurrentToken = T_Divide;
	return true;
}

bool Lexer::HandleWords()
{
	if( ! IsAlpha(C) )
		return false;

	std::string word;
	word.push_back(C);

	C = GetNextChar();

	while( IsAlpha(C) || IsDigit(C) )
	{
		word.push_back(C);
		C = GetNextChar();
	}

		 if( word == "true" )
	{
		mLastBool = true;
		mCurrentToken = T_Bool;
	}

	else if( word == "false" )
	{
		mLastBool = false;
		mCurrentToken = T_Bool;
	}

	else if( word == "if" )
		mCurrentToken = T_If;

	else if( word == "else" )
		mCurrentToken = T_Else;

	else if( word == "elif" )
		mCurrentToken = T_Elif;

	else if( word == "for" )
		mCurrentToken = T_For;

	else if( word == "in" )
		mCurrentToken = T_In;

	else if( word == "while" )
		mCurrentToken = T_While;

	else if( word == "this" )
		mCurrentToken = T_This;

	else if( word == "nil" )
		mCurrentToken = T_Nil;

	else if( word == "return" )
		mCurrentToken = T_Return;

	else if( word == "break" )
		mCurrentToken = T_Break;

	else if( word == "continue" )
		mCurrentToken = T_Continue;

	else if( word == "yield" )
		mCurrentToken = T_Yield;

	else if( word == "and" )
		mCurrentToken = T_And;

	else if( word == "or" )
		mCurrentToken = T_Or;

	else if( word == "xor" )
		mCurrentToken = T_Xor;

	else if( word == "not" )
		mCurrentToken = T_Not;

	else if( word == "_" )
		mCurrentToken = T_Underscore;

	else // must be an identifier
	{
		mLastIdentifier = word;
		mCurrentToken = T_Identifier;
	}

	return true;
}

bool Lexer::HandleNumbers()
{
	if( ! IsDigit(C) )
		return false;

	std::string number;
	number.push_back(C);

	C = GetNextChar();

	while( IsDigit(C) || C == '_' )
	{
		if( C != '_' )
			number.push_back(C);

		C = GetNextChar();
	}

	if( C != '.' ) // integer
	{
		mLastInteger = std::stoi(number);
		mCurrentToken = T_Integer;
		return true;
	}
	else // float
	{
		number.push_back('.');

		C = GetNextChar();

		if( C == '.' ) // the case of "123.." ranges maybe?!?!
		{}

		while( IsDigit(C) || C == '_' )
		{
			if( C != '_' )
				number.push_back(C);

			C = GetNextChar();
		}

		mLastFloat = std::stof(number);
		mCurrentToken = T_Float;
		return true;
	}

	return false;
}

bool Lexer::HandleSingleChar(const char ch, const Token t)
{
	if( C == ch )
	{
		mCurrentToken = t;
		C = GetNextChar();
		return true;
	}

	return false;
}

bool Lexer::HandleColumns()
{
	if( C != ':' )
		return false;

	C = GetNextChar(); // eat :

	if( C == ':' )
	{
		mCurrentToken = T_DoubleColumn;
		C = GetNextChar(); // eat :
		return true;
	}

	mCurrentToken = T_Column;
	return true;
}

bool Lexer::HandleString()
{
	if( C != '"' )
		return false;

	mLastString.clear();

	C = GetNextChar();

	while( C != '"' )
	{
		if( C == '\\' ){} // TODO: handle escaping...

		mLastString.push_back(C);
		C = GetNextChar();
	}

	C = GetNextChar(); // eat the last '"'

	mCurrentToken = T_String;
	return true;
}

bool Lexer::HandleSubstractOrArrow()
{
	if( C != '-' )
		return false;

	C = GetNextChar(); // eat -

	if( C == '=' )
	{
		C = GetNextChar(); // eat =
		mCurrentToken = T_AssignSubtract;
		return true;
	}

	if( C == '>' )
	{
		C = GetNextChar(); // eat >
		mCurrentToken = T_Arrow;
		return true;
	}

	mCurrentToken = T_Subtract;
	return true;
}

bool Lexer::HandleLessOrPush()
{
	if( C != '<' )
		return false;

	C = GetNextChar(); // eat <

	if( C == '=' )
	{
		C = GetNextChar(); // eat =
		mCurrentToken = T_LessEqual;
		return true;
	}

	if( C == '<' )
	{
		C = GetNextChar(); // eat <
		mCurrentToken = T_ArrayPushBack;
		return true;
	}

	mCurrentToken = T_Less;
	return true;
}

bool Lexer::HandleGreaterOrPop()
{
	if( C != '>' )
		return false;

	C = GetNextChar(); // eat >

	if( C == '=' )
	{
		C = GetNextChar(); // eat =
		mCurrentToken = T_GreaterEqual;
		return true;
	}

	if( C == '>' )
	{
		C = GetNextChar(); // eat >
		mCurrentToken = T_ArrayPopBack;
		return true;
	}

	mCurrentToken = T_Greater;
	return true;
}

bool Lexer::HandleCharAndEqual(char ch, Token withoutAssign, Token withAssign)
{
	if( C != ch )
		return false;

	C = GetNextChar(); // eat ch

	if( C == '=' )
	{
		C = GetNextChar(); // eat =
		mCurrentToken = withAssign;
		return true;
	}

	mCurrentToken = withoutAssign;
	return true;
}

bool Lexer::HandleDollarSign()
{
	if( C != '$' )
		return false;

	C = GetNextChar(); // eat $

	if( C == '$' )
	{
		C = GetNextChar(); // eat $
		mCurrentToken = T_ArgumentList;
		return true;
	}

	if( IsDigit(C) )
	{
		mLastArgumentIndex = unsigned(C - '0');

		C = GetNextChar(); // eat digit
		mCurrentToken = T_Argument;
		return true;
	}

	mLastArgumentIndex = 0;
	mCurrentToken = T_Argument;
	return true;
}

}
