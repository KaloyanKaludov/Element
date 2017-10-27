#ifndef _PARSER_H_INCLUDED_
#define _PARSER_H_INCLUDED_

#include <memory>
#include <vector>
#include <string>
#include "Tokens.h"
#include "Lexer.h"

namespace element
{

class Logger;
	
namespace ast
{
	struct Node;
	struct FunctionNode;
}


class Parser
{
public:		Parser(Logger& logger);

	auto	Parse(std::istream& input) -> std::unique_ptr<ast::FunctionNode>;

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
	ast::Node* ParseExpression();

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
	Logger&	mLogger;
	Lexer	mLexer;
};

}

#endif // _PARSER_H_INCLUDED_
