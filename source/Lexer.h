#ifndef _LEXER_H_INCLUDED_
#define _LEXER_H_INCLUDED_

#include <string>
#include "Tokens.h"
#include "Logger.h"

namespace element
{

class Lexer
{
public:					Lexer(Logger& logger);
						Lexer(std::istream& input, Logger& logger);

	void				SetInputStream(std::istream& input);
	
	Token				GetNextToken();
	Token				GetNextToken_IgnoreNewLine();
	Token				GetCurrentToken() const;

	void				RewindDueToMissingElse();
	
	const SourceCoords& GetCurrentCoords() const;
	
	const std::string& 	GetLastIdentifier()	const;
	const std::string& 	GetLastString() const;
	int					GetLastInteger() const;
	int					GetLastArgumentIndex() const;
	float				GetLastFloat() const;
	bool				GetLastBool() const;
	
	const char*			TokenAsString(Token token) const;
	const char*			GetCurrentTokenAsString() const;
	void				DebugPrintToken(Token token) const;
	
protected:
	char GetNextChar();
	
	bool HandleCommentOrDivision();
	bool HandleWords();
	bool HandleNumbers();
	bool HandleSingleChar(const char ch, const Token t);
	bool HandleColumns();
	bool HandleString();
	bool HandleSubstractOrArrow();
	bool HandleLessOrPush();
	bool HandleGreaterOrPop();
	bool HandleCharAndEqual(char ch, Token withoutAssign, Token withAssign);
	bool HandleDollarSign();
	
private:
	std::istream*	mInput;
	Logger&			mLogger;

	char			C;
	
	Token			mCurrentToken;
	
	SourceCoords	mCurrentCoords;
	int				mCurrentColumn;
	
	std::string		mLastCharsRead;

	std::string		mLastIdentifier;
	std::string		mLastString;
	int				mLastInteger;
	int				mLastArgumentIndex;
	float			mLastFloat;
	bool			mLastBool;
	
	bool			mShouldStartOverAgain;
};

}

#endif // _LEXER_H_INCLUDED_
