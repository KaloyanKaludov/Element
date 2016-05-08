#include "Parser.h"

#include <algorithm>
#include "AST.h"
#include "Lexer.h"
#include "Logger.h"
#include "Operators.h"

namespace element
{

Parser::Parser(Lexer& lexer, Logger& logger)
: mLexer(lexer)
, mLogger(logger)
{
}

ast::Node* Parser::ParseExpression()
{
	Token prevToken		= T_InvalidToken;
	Token currentToken	= mLexer.GetCurrentToken();
	
	// Check for left over expression termination symbols
	while( currentToken == T_NewLine || currentToken == T_Semicolumn )
		currentToken = mLexer.GetNextToken_IgnoreNewLine(); // eat \n or ;
	
	if( currentToken == T_EOF )
		return nullptr;
	
	// https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
	// https://en.wikipedia.org/wiki/Shunting-yard_algorithm
	std::vector<Operator>	operators;
	std::vector<ast::Node*>	operands;
	
	operators.push_back({T_InvalidToken, -1, ET_UnaryOperator, nullptr}); // sentinel
	SourceCoords	coords;
	ExpressionType	expressionType = ET_Unknown;
	
	bool primaryExpected = false;
	
	do // the actual parsing
	{
		primaryExpected = false;
		coords			= mLexer.GetCurrentCoords();
		expressionType	= CurrentExpressionType(prevToken, currentToken);
		
		switch( expressionType )
		{
		case ET_BinaryOperator:
		case ET_UnaryOperator:
		{
			const OperatorInfo& op = GetOperatorInfo(currentToken, expressionType == ET_BinaryOperator);
			
			while( 	(op.precedence < operators.back().precedence) ||
					(op.precedence == operators.back().precedence && !op.isRightAssociative) )
			{
				FoldOperatorStacks(operators, operands);
			}
			
			operators.push_back({
				currentToken, 
				op.precedence, 
				expressionType, 
				nullptr, 
				coords
			});
			
			primaryExpected = true;
			mLexer.GetNextToken_IgnoreNewLine();
			break;
		}
		
		case ET_IndexOperator:
		{
			const OperatorInfo& op = GetOperatorInfo(currentToken, expressionType == ET_BinaryOperator);
			
			while( 	(op.precedence < operators.back().precedence) ||
					(op.precedence == operators.back().precedence && !op.isRightAssociative) )
			{
				FoldOperatorStacks(operators, operands);
			}
			
			ast::Node* auxNode = ParseIndexOperator();
			
			if( ! auxNode ) // propagate error
			{
				for( ast::Node* trash : operands )
					delete trash;
				return nullptr;
			}
			
			operators.push_back({
				currentToken, 
				op.precedence, 
				expressionType, 
				auxNode, 
				coords
			});
			break;
		}
		
		case ET_FunctionCall:
		{
			const OperatorInfo& op = GetOperatorInfo(currentToken, expressionType == ET_BinaryOperator);
			
			while( 	(op.precedence < operators.back().precedence) ||
					(op.precedence == operators.back().precedence && !op.isRightAssociative) )
			{
				FoldOperatorStacks(operators, operands);
			}
			
			ast::Node* auxNode = ParseArguments();
			
			if( ! auxNode ) // propagate error
			{
				for( ast::Node* trash : operands )
					delete trash;
				return nullptr;
			}
			
			operators.push_back({
				currentToken, 
				op.precedence, 
				expressionType, 
				auxNode, 
				coords
			});
			break;
		}
		
		case ET_FunctionAssignment:
		{
			const OperatorInfo& op = GetOperatorInfo(currentToken, expressionType == ET_BinaryOperator);
			
			while( 	(op.precedence < operators.back().precedence) ||
					(op.precedence == operators.back().precedence && !op.isRightAssociative) )
			{
				FoldOperatorStacks(operators, operands);
			}
			
			ast::Node* auxNode = ParseFunction();
			
			if( ! auxNode ) // propagate error
			{
				for( ast::Node* trash : operands )
					delete trash;
				return nullptr;
			};
			
			operators.push_back({
				currentToken, 
				op.precedence, 
				expressionType, 
				auxNode, 
				coords
			});
			break;
		}
		
		case ET_PrimaryExpression:
		{
			ast::Node* node = ParsePrimary();
			
			if( ! node ) // propagate error
			{
				for( ast::Node* trash : operands )
					delete trash;
				return nullptr;
			}
			
			operands.push_back( node );
			break;
		}
		
		case ET_Unknown:
		default:
		{
			mLogger.PushError(coords, "Syntax error: operator expected");
			for( ast::Node* trash : operands )
				delete trash;
			return nullptr;
		}
		}
		
		prevToken	= currentToken;
		currentToken= mLexer.GetCurrentToken();
	}
	while( !IsExpressionTerminator(currentToken) || primaryExpected );
	
	
	// no more of this expression, fold everything to a single node
	while(	operands.size() > 1 || // last thing left is the result
			operators.size() > 1 ) // parse all, leave only the sentinel
	{
		FoldOperatorStacks(operators, operands);
	}
	
	return operands.back();
}

void Parser::DebugPrintAST(const ast::Node* root, int indent) const
{
	if( ! root )
		return;
	
	static const int TabSize = 2;

	std::string space;
	for( int i = 0; i < indent; ++i )
		space.push_back(' ');
	auto printSpace = [&](){ printf("%s", space.c_str()); };
	
	switch( root->type )
	{
	case ast::Node::N_Nil:
		{
			printSpace(); printf("Nil\n");
			break;
		}
	case ast::Node::N_Bool:
		{
			 ast::BoolNode* n = (ast::BoolNode*)root;
			 printSpace(); printf("Bool %s\n", n->value ? "true" : "false");
			 break;
		}
	case ast::Node::N_Integer:
		{
			ast::IntegerNode* n = (ast::IntegerNode*)root;
			printSpace(); printf("Int %d\n", n->value);
			break;
		}
	case ast::Node::N_Float:
		{
			ast::FloatNode* n = (ast::FloatNode*)root;
			printSpace(); printf("Float %f\n", n->value);
			break;
		}
	case ast::Node::N_String:
		{
			ast::StringNode* n = (ast::StringNode*)root;
			printSpace(); printf("String \"%s\"\n", n->value.c_str());
			break;
		}
	case ast::Node::N_Variable:
		{
			ast::VariableNode* n = (ast::VariableNode*)root;
			printSpace(); printf("Variable ");
			if( n->variableType == ast::VariableNode::V_Named )
				printf("%s\n", n->name.c_str());
			else if( n->variableType == ast::VariableNode::V_This )
				printf("this\n");
			else if( n->variableType == ast::VariableNode::V_ArgumentList )
				printf("argument list\n");
			else
				printf("argument at index %d\n", n->variableType);
			break;
		}
	case ast::Node::N_Arguments:
		{
			ast::ArgumentsNode* n = (ast::ArgumentsNode*)root;
			if( n->arguments.empty() )
			{
				printSpace(); printf("Empty argument list\n");
			}
			else
			{
				for( const auto& argument : n->arguments )
				{
					printSpace(); printf("Argument\n");
					DebugPrintAST(argument, indent + TabSize);
				}
			}
			break;
		}
	case ast::Node::N_UnaryOperator:
		{
			ast::UnaryOperatorNode* n = (ast::UnaryOperatorNode*)root;
			printSpace(); printf("Unary "); mLexer.DebugPrintToken(n->op);
			printf("\n");
			DebugPrintAST(n->operand, indent + TabSize);
			break;
		}
	case ast::Node::N_BinaryOperator:
		{
			ast::BinaryOperatorNode* n = (ast::BinaryOperatorNode*)root;
			printSpace();
			if( n->op != T_LeftBracket )
				mLexer.DebugPrintToken(n->op);
			else
				printf("[]");
			printf("\n");
			DebugPrintAST(n->lhs, indent + TabSize);
			DebugPrintAST(n->rhs, indent + TabSize);
			break;
		}
	case ast::Node::N_If:
		{
			ast::IfNode* n = (ast::IfNode*)root;
			printSpace(); printf("If\n");
			DebugPrintAST(n->condition, indent + TabSize);
			printSpace(); printf("Then\n");
			DebugPrintAST(n->thenPath, indent + TabSize);
			if( n->elsePath )
			{
				printSpace(); printf("Else\n");
				DebugPrintAST(n->elsePath, indent + TabSize);
			}
			break;
		}
	case ast::Node::N_While:
		{
			ast::WhileNode* n = (ast::WhileNode*)root;
			printSpace(); printf("While\n");
			DebugPrintAST(n->condition, indent + TabSize);
			printSpace(); printf("Do\n");
			DebugPrintAST(n->body, indent + TabSize);
			break;
		}
	case ast::Node::N_For:
		{
			ast::ForNode* n = (ast::ForNode*)root;
			printSpace(); printf("For\n");
			DebugPrintAST(n->iterator, indent + TabSize);
			printSpace(); printf("In\n");
			DebugPrintAST(n->iteratedExpression, indent + TabSize);
			printSpace(); printf("Do\n");
			DebugPrintAST(n->body, indent + TabSize);
			break;
		}
	case ast::Node::N_Block:
		{
			ast::BlockNode* n = (ast::BlockNode*)root;
			printSpace(); printf("{\n");
			for( const auto& it : n->nodes )
				DebugPrintAST(it, indent + TabSize);
			printSpace(); printf("}\n");
			break;
		}
	case ast::Node::N_Array:
		{
			ast::ArrayNode* n = (ast::ArrayNode*)root;
			printSpace(); printf("[\n");
			for( const auto& it : n->elements )
				DebugPrintAST(it, indent + TabSize);
			printSpace(); printf("]\n");
			break;
		}
	case ast::Node::N_Object:
		{
			ast::ObjectNode* n = (ast::ObjectNode*)root;
			if( n->members.empty() )
			{
				printSpace(); printf("[=]\n");
			}
			else
			{
				printSpace(); printf("[\n");
				for( const auto& member : n->members )
				{
					printSpace(); printf("Key\n");
					DebugPrintAST(member.first, indent + TabSize);
					printSpace(); printf("Value\n");
					DebugPrintAST(member.second, indent + TabSize);
				}
				printSpace(); printf("]\n");
			}
			break;
		}
	case ast::Node::N_Function:
		{
			ast::FunctionNode* n = (ast::FunctionNode*)root;
			printSpace(); printf("Function (");
			size_t sz = n->namedParameters.size();
			for( size_t i = 0; i < sz; ++i )
				printf(i == sz - 1 ? "%s" : "%s, ", n->namedParameters[i].c_str());
			printf(")\n");
			DebugPrintAST(n->body, indent + TabSize);
			break;
		}
	case ast::Node::N_FunctionCall:
		{
			ast::FunctionCallNode* n = (ast::FunctionCallNode*)root;
			printSpace(); printf("FunctionCall\n");
			DebugPrintAST(n->function, indent + TabSize);
			DebugPrintAST(n->arguments, indent + TabSize);
			break;
		}
	case ast::Node::N_Return:
		{
			ast::ReturnNode* n = (ast::ReturnNode*)root;
			printSpace(); printf("Return\n");
			if( n->value )
				DebugPrintAST(n->value, indent + TabSize);
			break;
		}
	case ast::Node::N_Break:
		{
			ast::BreakNode* n = (ast::BreakNode*)root;
			printSpace(); printf("Break\n");
			if( n->value )
				DebugPrintAST(n->value, indent + TabSize);
			break;
		}
	case ast::Node::N_Continue:
		{
			ast::ContinueNode* n = (ast::ContinueNode*)root;
			printSpace(); printf("Continue\n");
			if( n->value )
				DebugPrintAST(n->value, indent + TabSize);
			break;
		}
	default:
		{}
	}
}

ast::Node* Parser::ParsePrimary()
{
	switch( mLexer.GetCurrentToken() )
	{
	case T_Nil:
	case T_Integer:
	case T_Float:
	case T_String:
	case T_Bool:
		return ParsePrimitive();
	
	case T_This:
	case T_Argument:
	case T_ArgumentList:
	case T_Identifier:
		return ParseVarialbe();

	case T_LeftParent:
		return ParseParenthesis();

	case T_LeftBrace:
		return ParseBlock();
	
	case T_LeftBracket:
		return ParseArrayOrObject();
	
	case T_Column:
	case T_DoubleColumn:
		return ParseFunction();
	
	case T_If:
		return ParseIf();
	
	case T_While:
		return ParseWhile();
	
	case T_For:
		return ParseFor();
	
	case T_Return:
	case T_Break:
	case T_Continue:
		return ParseControlExpression();
	
	default:
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: unexpected token %s", mLexer.GetCurrentTokenAsString());
		return nullptr;
	}
}

ast::Node* Parser::ParsePrimitive()
{
	SourceCoords coords = mLexer.GetCurrentCoords();

	switch( mLexer.GetCurrentToken() )
	{
	case T_Nil:
		{
			mLexer.GetNextToken(); // eat nil
			return new ast::Node(ast::Node::N_Nil, coords);
		}
	case T_Integer:
		{
			int i = mLexer.GetLastInteger();
			mLexer.GetNextToken(); // eat integer
			return new ast::IntegerNode(i, coords);
		}
	case T_Float:
		{
			float f = mLexer.GetLastFloat();
			mLexer.GetNextToken(); // eat float
			return new ast::FloatNode(f, coords);
		}
	case T_String:
		{
			std::string s = mLexer.GetLastString();
			mLexer.GetNextToken(); // eat string
			return new ast::StringNode(s, coords);
		}
	case T_Bool:
		{
			bool b = mLexer.GetLastBool();
			mLexer.GetNextToken(); // eat bool
			return new ast::BoolNode(b, coords);
		}
	default:
		mLogger.PushError(coords, "Syntax error: unexpected token %s", mLexer.GetCurrentTokenAsString());
		return nullptr;
	}
}

ast::Node* Parser::ParseVarialbe()
{
	SourceCoords coords = mLexer.GetCurrentCoords();

	switch( mLexer.GetCurrentToken() )
	{
	case T_This:
		{
			mLexer.GetNextToken(); // eat this
			return new ast::VariableNode(ast::VariableNode::V_This, coords);
		}
	case T_Argument:
		{
			int n = mLexer.GetLastArgumentIndex();
			mLexer.GetNextToken(); // eat $
			return new ast::VariableNode(n, coords);
		}
	case T_ArgumentList:
		{
			mLexer.GetNextToken(); // eat $$
			return new ast::VariableNode(ast::VariableNode::V_ArgumentList, coords);
		}
	case T_Identifier:
		{
			std::string s = mLexer.GetLastIdentifier();
			mLexer.GetNextToken(); // eat identifier
			return new ast::VariableNode(s, coords);
		}
	default:
		mLogger.PushError(coords, "Syntax error: unexpected token %s", mLexer.GetCurrentTokenAsString());
		return nullptr;
	}
}

ast::Node* Parser::ParseParenthesis()
{
	mLexer.GetNextToken_IgnoreNewLine(); // eat (
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		return nullptr;
	}
	
	ast::Node* node = ParseExpression();
	
	if( ! node )
		return nullptr;
	
	if( mLexer.GetCurrentToken() != T_RightParent )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected )");
		delete node;
		return nullptr;
	}
	
	mLexer.GetNextToken(); // eat )
	
	return node;
}

ast::Node* Parser::ParseIndexOperator()
{
	mLexer.GetNextToken_IgnoreNewLine(); // eat [
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		return nullptr;
	}
	
	ast::Node* node = ParseExpression();
	
	if( ! node )
		return nullptr;
	
	if( mLexer.GetCurrentToken() != T_RightBracket )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected ]");
		delete node;
		return nullptr;
	}
	
	mLexer.GetNextToken(); // eat ]
	
	return node;
}

ast::Node* Parser::ParseBlock()
{
	SourceCoords coords = mLexer.GetCurrentCoords();

	mLexer.GetNextToken_IgnoreNewLine(); // eat {
		
	std::vector<ast::Node*> nodes;
	
	while( mLexer.GetCurrentToken() != T_RightBrace )
	{
		ast::Node* node = ParseExpression();
		
		if( ! node )
		{
			for( ast::Node* trash : nodes )
				delete trash;
			return nullptr;
		}
		
		nodes.push_back( node );
		
		// Check for left over expression termination symbols
		Token token = mLexer.GetCurrentToken();
		while( token == T_NewLine || token == T_Semicolumn )
			token = mLexer.GetNextToken_IgnoreNewLine(); // eat \n or ;
	}
	
	mLexer.GetNextToken(); // eat }
	
	return new ast::BlockNode(nodes, coords);
}

ast::Node* Parser::ParseFunction()
{
	SourceCoords coords = mLexer.GetCurrentCoords();
	std::vector<std::string> namedParameters;
	
	if( mLexer.GetCurrentToken() == T_DoubleColumn )
	{
		mLexer.GetNextToken_IgnoreNewLine(); // eat ::
	}
	else if( mLexer.GetCurrentToken() == T_Column )
	{
		mLexer.GetNextToken_IgnoreNewLine(); // eat :
		
		if( mLexer.GetCurrentToken() != T_LeftParent )
		{
			mLogger.PushError(coords, "Syntax error: expected (");
			return nullptr;
		}
				
		mLexer.GetNextToken_IgnoreNewLine(); // eat (
		
		while( mLexer.GetCurrentToken() != T_RightParent )
		{
			if( mLexer.GetCurrentToken() != T_Identifier )
			{
				mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: identifier expected");
				return nullptr;
			}
			
			namedParameters.push_back( mLexer.GetLastIdentifier() );
			
			Token token = mLexer.GetNextToken_IgnoreNewLine(); // eat identifier
			
			if( token == T_Comma )
			{
				mLexer.GetNextToken_IgnoreNewLine(); // eat ,
			}
			else if( token != T_RightParent )
			{
				mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected , or )");
				return nullptr;
			}
		}
		
		mLexer.GetNextToken_IgnoreNewLine(); // eat )
	}
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		return nullptr;
	}
	
	ast::Node* body = ParseExpression();
	
	if( ! body )
		return nullptr;
	
	return new ast::FunctionNode(namedParameters, body, coords);
}

ast::Node* Parser::ParseArguments()
{
	SourceCoords coords = mLexer.GetCurrentCoords();
	mLexer.GetNextToken_IgnoreNewLine(); // eat (
	
	std::vector<ast::Node*> arguments;
	
	while( mLexer.GetCurrentToken() != T_RightParent )
	{
		if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
		{
			mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
			for( ast::Node* trash : arguments )
				delete trash;
			return nullptr;
		}

		ast::Node* argument = ParseExpression();
		
		if( ! argument )
		{
			for( ast::Node* trash : arguments )
				delete trash;
			return nullptr;
		}
		
		arguments.push_back( argument );
		
		if( mLexer.GetCurrentToken() == T_NewLine )
			mLexer.GetNextToken_IgnoreNewLine(); // eat \n
		
		if( mLexer.GetCurrentToken() == T_Comma )
		{
			mLexer.GetNextToken_IgnoreNewLine(); // eat ,
		}
		else if( mLexer.GetCurrentToken() != T_RightParent )
		{
			mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected )");
			for( ast::Node* trash : arguments )
				delete trash;
			return nullptr;
		}
	}
	
	mLexer.GetNextToken(); // eat )
	
	return new ast::ArgumentsNode(arguments, coords);
}

ast::Node* Parser::ParseArrayOrObject()
{
	SourceCoords coords = mLexer.GetCurrentCoords();
	mLexer.GetNextToken_IgnoreNewLine(); // eat [
	
	// check for empty object literal [=] //////////////////////////////////////
	if( mLexer.GetCurrentToken() == T_Assignment )
	{
		mLexer.GetNextToken_IgnoreNewLine(); // eat =
		
		if( mLexer.GetCurrentToken() == T_RightBracket )
		{
			mLexer.GetNextToken(); // eat ]
			return new ast::ObjectNode(coords);
		}
		else
		{
			mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
			return nullptr;
		}
	}
	
	// otherwise it could be either an array or an object with members /////////
	auto IsAssignmentOperator = [](const ast::Node* n)
	{
		return	n->type == ast::Node::N_BinaryOperator &&
				((ast::BinaryOperatorNode*)n)->op == T_Assignment;
	};

	bool firstExpression= true;
	bool isObject		= false;
	
	std::vector<ast::Node*> keys;
	std::vector<ast::Node*> elements;
	
	while( mLexer.GetCurrentToken() != T_RightBracket )
	{
		if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
		{
			mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
			for( ast::Node* trash : keys )		delete trash;
			for( ast::Node* trash : elements )	delete trash;
			return nullptr;
		}
		
		ast::Node* element = ParseExpression();
		
		if( ! element )
		{
			for( ast::Node* trash : keys )		delete trash;
			for( ast::Node* trash : elements )	delete trash;
			return nullptr;
		}
		
		if( firstExpression )
		{
			isObject = IsAssignmentOperator(element);
			firstExpression = false;
		}
		else if( isObject != IsAssignmentOperator(element) )
		{
			mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: mixing together syntax for arrays and objects");
			for( ast::Node* trash : keys )		delete trash;
			for( ast::Node* trash : elements )	delete trash;
			return nullptr;
		}
		
		if( isObject )
		{
			ast::BinaryOperatorNode* node = (ast::BinaryOperatorNode*)element;
			keys.push_back( node->lhs );
			elements.push_back( node->rhs );
		}
		else
		{
			elements.push_back( element );
		}
		
		Token trailingToken = mLexer.GetCurrentToken();
		while( trailingToken == T_NewLine )
			trailingToken = mLexer.GetNextToken_IgnoreNewLine(); // eat \n
		
		if( trailingToken == T_Comma )
		{
			mLexer.GetNextToken_IgnoreNewLine(); // eat ,
		}
		else if( trailingToken != T_RightBracket )
		{
			mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected, elements should be separated by commas");
			for( ast::Node* trash : keys )		delete trash;
			for( ast::Node* trash : elements )	delete trash;
			return nullptr;
		}
	}
	
	mLexer.GetNextToken(); // eat ]
	
	if( isObject )
	{
		size_t size = elements.size();
		
		ast::ObjectNode::KeyValuePairs pairs;
		pairs.reserve(size);
		
		for( size_t i = 0; i < size; ++i )
			pairs.push_back(std::make_pair(keys[i], elements[i]));
		
		return new ast::ObjectNode(pairs, coords);
	}
	else
	{
		return new ast::ArrayNode(elements, coords);
	}
}

ast::Node* Parser::ParseIf()
{
	SourceCoords coords = mLexer.GetCurrentCoords();
	mLexer.GetNextToken_IgnoreNewLine(); // eat if
	
	if( mLexer.GetCurrentToken() != T_LeftParent )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected (");
		return nullptr; // Syntax error
	}
	
	mLexer.GetNextToken_IgnoreNewLine(); // eat (
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		return nullptr;
	}
	
	ast::Node* condition = ParseExpression();
	
	if( ! condition )
		return nullptr;
	
	if( mLexer.GetCurrentToken() != T_RightParent )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected )");
		delete condition;
		return nullptr; // Syntax error
	}
	
	mLexer.GetNextToken_IgnoreNewLine(); // eat )
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		delete condition;
		return nullptr;
	}
	
	ast::Node* thenPath = ParseExpression();
	
	if( ! thenPath ) // propagate error
	{
		delete condition;
		return nullptr;
	}
	
	ast::Node* elsePath = nullptr;
	
	// HACK ////////////////////////////////////////////////////////////////////
	// we need to check if there will be an 'else' clause, but it may be after
	// the new line, so we have to eat it. This is troublesome because later the
	// 'ParseExpression' function will not know to terminate the expression.
	// To prevent this we will "rewind" the lexer input back to its original state.
	bool shouldRewind = true;
	if( mLexer.GetCurrentToken() == T_NewLine )
		mLexer.GetNextToken_IgnoreNewLine(); // eat \n
		
	if( mLexer.GetCurrentToken() == T_Elif )
	{
		elsePath = ParseIf();
		
		if( ! elsePath )
		{
			delete condition;
			delete thenPath;
			return nullptr;
		}

		shouldRewind = false;
	}
	else if( mLexer.GetCurrentToken() == T_Else )
	{
		mLexer.GetNextToken_IgnoreNewLine(); // eat else
	
		if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
		{
			mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
			delete condition;
			delete thenPath;
			return nullptr;
		}
		
		elsePath = ParseExpression();
		
		if( ! elsePath )
		{
			delete condition;
			delete thenPath;
			return nullptr;
		}

		shouldRewind = false;
	}

	if( shouldRewind )
		mLexer.RewindDueToMissingElse();
	
	return new ast::IfNode(condition, thenPath, elsePath, coords);
}

ast::Node* Parser::ParseWhile()
{
	SourceCoords coords = mLexer.GetCurrentCoords();
	mLexer.GetNextToken_IgnoreNewLine(); // eat while
	
	if( mLexer.GetCurrentToken() != T_LeftParent )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected (");
		return nullptr;
	}
	
	mLexer.GetNextToken_IgnoreNewLine(); // eat (
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		return nullptr;
	}
	
	ast::Node* condition = ParseExpression();
	
	if( ! condition )
		return nullptr;
	
	if( mLexer.GetCurrentToken() != T_RightParent )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected )");
		delete condition;
		return nullptr;
	}
	
	mLexer.GetNextToken_IgnoreNewLine(); // eat )
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		delete condition;
		return nullptr;
	}
	
	ast::Node* body = ParseExpression();
	
	if( ! body ) // propagate error
	{
		delete condition;
		return nullptr;
	}
	
	return new ast::WhileNode(condition, body, coords);
}

ast::Node* Parser::ParseFor()
{
	SourceCoords coords = mLexer.GetCurrentCoords();
	mLexer.GetNextToken_IgnoreNewLine(); // eat for
	
	if( mLexer.GetCurrentToken() != T_LeftParent )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected (");
		return nullptr;
	}
	
	mLexer.GetNextToken_IgnoreNewLine(); // eat (
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		return nullptr;
	}
	
	ast::Node* iteratorVariable = ParseExpression();
	
	if( ! iteratorVariable )
		return nullptr;
	
	if( mLexer.GetCurrentToken() != T_In )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected 'in'");
		delete iteratorVariable;
		return nullptr;
	}
	
	mLexer.GetNextToken_IgnoreNewLine(); // eat in
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		delete iteratorVariable;
		return nullptr;
	}
	
	ast::Node* iteratedExpression = ParseExpression();
	
	if( ! iteratedExpression )
	{
		delete iteratorVariable;
		return nullptr;
	}
	
	if( mLexer.GetCurrentToken() != T_RightParent )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expected )");
		delete iteratorVariable;
		delete iteratedExpression;
		return nullptr;
	}
	
	mLexer.GetNextToken_IgnoreNewLine(); // eat )
	
	if( IsExpressionTerminator(mLexer.GetCurrentToken()) )
	{
		mLogger.PushError(mLexer.GetCurrentCoords(), "Syntax error: expression expected");
		delete iteratorVariable;
		delete iteratedExpression;
		return nullptr;
	}
	
	ast::Node* body = ParseExpression();
	
	if( ! body ) // propagate error
	{
		delete iteratorVariable;
		delete iteratedExpression;
		return nullptr;
	}
	
	return new ast::ForNode(iteratorVariable, iteratedExpression, body, coords);
}

ast::Node* Parser::ParseControlExpression()
{
	SourceCoords coords = mLexer.GetCurrentCoords();
	Token controlType = mLexer.GetCurrentToken();
	
	Token token = mLexer.GetNextToken(); // eat the control expression
	
	ast::Node* value = nullptr;
	
	if( ! IsExpressionTerminator(token) )
	{
		value = ParseExpression();
		
		if( ! value ) // propagate error
			return nullptr;
	}
	
	switch( controlType )
	{
	case T_Return:	return new ast::ReturnNode	(value, coords);
	case T_Break:	return new ast::BreakNode	(value, coords);
	case T_Continue:return new ast::ContinueNode(value, coords);
	default:
		delete value;
		return nullptr;
	}
}

Parser::ExpressionType Parser::CurrentExpressionType(Token prevToken, Token token) const
{
	static const Token operators[] = {
		T_Add, T_Subtract, T_Divide, T_Multiply, T_Power, T_Modulo, T_Concatenate,
		T_Assignment, 
		T_AssignAdd, T_AssignSubtract, T_AssignDivide, T_AssignMultiply, 
		T_AssignPower, T_AssignModulo, T_AssignConcatenate,
		T_Dot, T_Arrow, T_ArrayPushBack, T_ArrayPopBack,
		T_Equal, T_NotEqual, T_Less, T_Greater, T_LessEqual, T_GreaterEqual,
		T_And, T_Or, T_Xor, T_Not, T_SizeOf,
	};
	
	auto isOperator = [&](const Token t) -> bool
	{
		for( const Token o : operators )
			if( t == o )
				return true;
		return false;
	};
	
	static const Token unaryOperators[] = {
		T_Not, T_SizeOf, T_Subtract, T_Add, T_Concatenate,
	};
	
	auto isUnaryOperator = [&](const Token t) -> bool
	{
		for( const Token o : unaryOperators )
			if( t == o )
				return true;
		return false;
	};
	
	// weird logic follows...
	
	if( prevToken == T_InvalidToken ) // no previous token
	{
		if( isUnaryOperator(token) )
			return ET_UnaryOperator;
		else
			return ET_PrimaryExpression;
	}
	else
	{
		if( isOperator(prevToken) )
		{
			if( isUnaryOperator(token) )
				return ET_UnaryOperator;
			else
				return ET_PrimaryExpression;
		}
		else
		{
			if(	token == T_LeftBracket )
				return ET_IndexOperator;
			
			if(	token == T_LeftParent )
				return ET_FunctionCall;
			
			if(	token == T_Column || token == T_DoubleColumn )
				return ET_FunctionAssignment;
			
			if( isOperator(token) && token != T_Not && token != T_SizeOf )
				return ET_BinaryOperator;
		}
	}
	
	return ET_Unknown;
}

void Parser::FoldOperatorStacks(std::vector<Operator>& operators, std::vector<ast::Node*>& operands) const
{
	ast::Node* newNode = nullptr;
	
	Operator topOperator = operators.back();
	operators.pop_back();
	
	switch( topOperator.type )
	{
	case ET_BinaryOperator:
		{
			ast::Node* rhs = operands.back();
			operands.pop_back();
			ast::Node* lhs = operands.back();
			operands.pop_back();
			
			newNode = new ast::BinaryOperatorNode(topOperator.token, lhs, rhs, topOperator.coords);
			break;
		}
	
	case ET_UnaryOperator:
		{
			ast::Node* operand = operands.back();
			operands.pop_back();
			
			newNode = new ast::UnaryOperatorNode(topOperator.token, operand, topOperator.coords);
			break;
		}
	
	case ET_IndexOperator:
		{
			ast::Node* operand = operands.back();
			operands.pop_back();
			
			newNode = new ast::BinaryOperatorNode(topOperator.token, operand, topOperator.auxNode, topOperator.coords);
			break;
		}
	
	case ET_FunctionCall:
		{
			ast::Node* function = operands.back();
			operands.pop_back();
			
			newNode = new ast::FunctionCallNode(function, topOperator.auxNode, topOperator.coords);
			break;
		}
	
	case ET_FunctionAssignment:
		{
			ast::Node* lhs = operands.back();
			operands.pop_back();
			
			newNode = new ast::BinaryOperatorNode(T_Assignment, lhs, topOperator.auxNode, topOperator.coords);
			break;
		}
	default:
		break;
	}
	
	operands.push_back(newNode);
}

bool Parser::IsExpressionTerminator(Token token) const
{
	return 	token == T_NewLine		|| 
			token == T_Semicolumn	|| 
			token == T_Comma		|| 
			token == T_RightParent	|| 
			token == T_RightBracket	|| 
			token == T_RightBrace	|| 
			token == T_Else			||
			token == T_Elif			||
			token == T_In			||
			token == T_EOF;
}

}
