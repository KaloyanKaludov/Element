#ifndef _PARSER_H_INCLUDED_
#define _PARSER_H_INCLUDED_

#include <vector>
#include <string>
#include "Tokens.h"
#include "Logger.h"

namespace element
{

namespace ast
{
	struct Node;
}
class Lexer;


class Parser
{
public:			Parser(Lexer& lexer, Logger& logger);
	
	ast::Node*	ParseExpression();
	
	void		DebugPrintAST(const ast::Node* root, int indent = 0) const;

protected:
	enum ExpressionType
	{
		ET_PrimaryExpression,
		ET_UnaryOperator,
		ET_BinaryOperator,
		ET_IndexOperator,
		ET_FunctionCall,
		ET_FunctionAssignment,

		ET_Unknown,
	};
	
	struct Operator
	{
		Token 			token;
		int 			precedence;
		ExpressionType	type;
		ast::Node*		auxNode;
		SourceCoords	coords;
	};

protected:
	ast::Node* ParsePrimary();
	ast::Node* ParsePrimitive();
	ast::Node* ParseVarialbe();
	ast::Node* ParseParenthesis();
	ast::Node* ParseIndexOperator();
	ast::Node* ParseBlock();
	ast::Node* ParseFunction();
	ast::Node* ParseArguments();
	ast::Node* ParseArrayOrObject();
	ast::Node* ParseIf();
	ast::Node* ParseWhile();
	ast::Node* ParseFor();
	ast::Node* ParseControlExpression();
	
	ExpressionType CurrentExpressionType(Token prevToken, Token token) const;

	void FoldOperatorStacks(std::vector<Operator>& operators, 
							std::vector<ast::Node*>& operands) const;
	
	bool IsExpressionTerminator(Token token) const;
	
private:
	Lexer&	mLexer;
	Logger& mLogger;
};

}

#endif // _PARSER_H_INCLUDED_
