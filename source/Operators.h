#ifndef _PRECEDENCE_H_INCLUDED_
#define _PRECEDENCE_H_INCLUDED_

#include "Tokens.h"

namespace element
{

struct OperatorInfo
{
	Token	token;
	bool	isBinary;
	bool	isRightAssociative;
	int		precedence;
};

const OperatorInfo& GetOperatorInfo(const Token t, bool isBinary);

}

#endif // _PRECEDENCE_H_INCLUDED_
