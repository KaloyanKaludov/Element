#include "SemanticAnalyzer.h"

#include "Logger.h"

namespace element
{

SemanticAnalyzer::SemanticAnalyzer(Logger& logger)
: mLogger(logger)
{
}

void SemanticAnalyzer::Analyze(std::vector<ast::Node*>& astNodes)
{
	mContext.push_back(CXT_InGlobal);
	
	for( auto& node : astNodes )
		if( ! AnalyzeNode(node) )
			break;
	
	mContext.pop_back();
}

bool SemanticAnalyzer::AnalyzeNode(ast::Node* node)
{
	if( ! node )
		return false;
	
	switch( node->type )
	{
	case ast::Node::N_Nil:
	case ast::Node::N_Bool:
	case ast::Node::N_Integer:
	case ast::Node::N_Float:
	case ast::Node::N_String:
	case ast::Node::N_Variable:
		return true;
	
	case ast::Node::N_Arguments:
	{
		ast::ArgumentsNode* n = (ast::ArgumentsNode*)node;
		
		mContext.push_back(CXT_InArguments);
		
		for( auto& argument : n->arguments )
			if( ! AnalyzeNode(argument) )
				return false;
		
		mContext.pop_back();
		
		return true;
	}
	
	case ast::Node::N_UnaryOperator:
	{
		ast::UnaryOperatorNode* n = (ast::UnaryOperatorNode*)node;
		
		if( IsBreakContinueReturn(n->operand) )
		{
			mLogger.PushError(node->coords, "break, continue, return cannot be arguments to unary operators");
			return false;
		}

		return AnalyzeNode(n->operand);
	}
	
	case ast::Node::N_BinaryOperator:
	{
		ast::BinaryOperatorNode* n = (ast::BinaryOperatorNode*)node;
		
		if( n->op != Token::T_And && 
			n->op != Token::T_Or &&
			(IsBreakContinueReturn(n->lhs) || IsBreakContinueReturn(n->rhs)) )
		{
			mLogger.PushError(node->coords, "break, continue, return can only be used with 'and' and 'or' operators");
			return false;
		}
		
		int lhsType = n->lhs->type;
		
		if( n->op == Token::T_Assignment		||
			n->op == Token::T_AssignAdd			||
			n->op == Token::T_AssignSubtract	||
			n->op == Token::T_AssignMultiply	||
			n->op == Token::T_AssignDivide		||
			n->op == Token::T_AssignConcatenate	||
			n->op == Token::T_AssignPower		||
			n->op == Token::T_AssignModulo )
		{
			if( lhsType == ast::Node::N_Variable )
			{
				ast::VariableNode* var = (ast::VariableNode*)n->lhs;
				
				if( var->variableType == ast::VariableNode::V_This )
				{
					mLogger.PushError(var->coords, "the 'this' variable is not assignable");
					return false;
				}
				if( var->variableType == ast::VariableNode::V_ArgumentList )
				{
					mLogger.PushError(var->coords, "the $$ array is not assignable");
					return false;
				}
				if( var->variableType >= 0 ) // anonymous parameter
				{
					mLogger.PushError(var->coords, "the $%d variable is not assignable", var->variableType);
					return false;
				}
			}
			else if( lhsType == ast::Node::N_BinaryOperator )
			{
				ast::BinaryOperatorNode* bin = (ast::BinaryOperatorNode*)n->lhs;
				
				if( bin->op != Token::T_LeftBracket &&	// array access
					bin->op != Token::T_Dot )			// member access
				{
					mLogger.PushError(bin->coords, "assignment must have a variable on the left hand side");
					return false;
				}
				
				if(	bin->op == Token::T_LeftBracket &&
					bin->lhs->type == ast::Node::N_Variable &&
					((ast::VariableNode*)bin->lhs)->variableType == ast::VariableNode::V_ArgumentList )
				{
					mLogger.PushError(bin->coords, "elements of the $$ array are not assignable");
					return false;
				}
			}
			else
			{
				mLogger.PushError(n->lhs->coords, "assignment must have a variable on the left hand side");
				return false;
			}
		}
		
		if( n->op == Token::T_Assignment &&
			n->rhs->type == ast::Node::N_Variable &&
			((ast::VariableNode*)n->rhs)->variableType == ast::VariableNode::V_ArgumentList )
		{
			mLogger.PushError(n->rhs->coords, "argument arrays cannot be assigned to variables, they must be copied");
			return false;
		}
		
		if( n->op == Token::T_LeftBracket )
		{
			if( lhsType == ast::Node::N_Nil			||
				lhsType == ast::Node::N_Integer		||
				lhsType == ast::Node::N_Float		||
				lhsType == ast::Node::N_Bool		||
				lhsType == ast::Node::N_Function	||
				lhsType == ast::Node::N_Arguments	||
				lhsType == ast::Node::N_Return		||
				lhsType == ast::Node::N_Break		||
				lhsType == ast::Node::N_Continue )
			{
				mLogger.PushError(n->lhs->coords, "invalid target for index operation");
				return false;
			}
		}
		
		if( n->op == Token::T_Dot )
		{
			if( lhsType == ast::Node::N_Nil			||
				lhsType == ast::Node::N_Integer		||
				lhsType == ast::Node::N_Float		||
				lhsType == ast::Node::N_Bool		||
				lhsType == ast::Node::N_String		||
				lhsType == ast::Node::N_Array		||
				lhsType == ast::Node::N_Function	||
				lhsType == ast::Node::N_Arguments	||
				lhsType == ast::Node::N_Return		||
				lhsType == ast::Node::N_Break		||
				lhsType == ast::Node::N_Continue )
			{
				mLogger.PushError(n->lhs->coords, "invalid target for member access");
				return false;
			}
		}
		
		return AnalyzeNode(n->lhs) && AnalyzeNode(n->rhs);
	}
	
	case ast::Node::N_If:
	{
		ast::IfNode* n = (ast::IfNode*)node;
		
		if( n->elsePath )
			return	AnalyzeNode(n->condition)	&&
					AnalyzeNode(n->thenPath)	&&
					AnalyzeNode(n->elsePath);
		else
			return	AnalyzeNode(n->condition)	&&
					AnalyzeNode(n->thenPath);
	}

	case ast::Node::N_While:
	{
		ast::WhileNode* n = (ast::WhileNode*)node;
		
		mContext.push_back(CXT_InLoop);
		
		bool ok = AnalyzeNode(n->condition) && AnalyzeNode(n->body);
		
		mContext.pop_back();
		
		return ok;
	}

	case ast::Node::N_For:
	{
		ast::ForNode* n = (ast::ForNode*)node;
		
		mContext.push_back(CXT_InLoop);
		
		bool ok = 	AnalyzeNode(n->iterator)			&&
					AnalyzeNode(n->iteratedExpression)	&&
					AnalyzeNode(n->body);
		
		mContext.pop_back();
		
		return ok;
	}
	
	case ast::Node::N_Block:
	{
		ast::BlockNode* n = (ast::BlockNode*)node;
		
		for( auto& it : n->nodes )
			if( ! AnalyzeNode(it) )
				return false;
		
		return true;
	}
	
	case ast::Node::N_Array:
	{
		ast::ArrayNode* n = (ast::ArrayNode*)node;
		
		mContext.push_back(CXT_InArray);
		
		for( auto& it : n->elements )
			if( ! AnalyzeNode(it) )
				return false;
		
		mContext.pop_back();
		
		return true;
	}
	
	case ast::Node::N_Object:
	{
		ast::ObjectNode* n = (ast::ObjectNode*)node;
		
		mContext.push_back(CXT_InObject);
		
		for( auto& it : n->members )
			if( ! (AnalyzeNode(it.first) && AnalyzeNode(it.second)) )
				return false;
		
		mContext.pop_back();
		
		return true;
	}
	
	case ast::Node::N_Function:
	{
		ast::FunctionNode* n = (ast::FunctionNode*)node;
		
		mContext.push_back(ContextType::CXT_InFunction);
		
		bool ok = AnalyzeNode(n->body);
		
		mContext.pop_back();
		
		return ok;
	}

	case ast::Node::N_FunctionCall:
	{
		ast::FunctionCallNode* n = (ast::FunctionCallNode*)node;
		
		int nodeType = n->function->type;
		
		if( nodeType == ast::Node::N_Nil		||
			nodeType == ast::Node::N_Integer	||
			nodeType == ast::Node::N_Float		||
			nodeType == ast::Node::N_Bool		||
			nodeType == ast::Node::N_String		||
			nodeType == ast::Node::N_Array		||
			nodeType == ast::Node::N_Object		||
			nodeType == ast::Node::N_Arguments	||
			nodeType == ast::Node::N_Return		||
			nodeType == ast::Node::N_Break		||
			nodeType == ast::Node::N_Continue )
		{
			mLogger.PushError(n->coords, "invalid target for function call");
			return false;
		}
		
		return AnalyzeNode(n->function) && AnalyzeNode(n->arguments);
	}

	case ast::Node::N_Return:
	{
		ast::ReturnNode* n = (ast::ReturnNode*)node;
		
		if( IsInConstruction() )
		{
			mLogger.PushError(n->coords, "return cannot be used inside array, object or argument constructions");
			return false;
		}
		
		if( ! IsInFunction() )
		{
			mLogger.PushError(n->coords, "return can only be used inside a function");
			return false;
		}
		
		if( n->value )
			return AnalyzeNode(n->value);
		
		return true;
	}
	
	case ast::Node::N_Break:
	{
		ast::BreakNode* n = (ast::BreakNode*)node;
		
		if( IsInConstruction() )
		{
			mLogger.PushError(n->coords, "break cannot be used inside array, object or argument constructions");
			return false;
		}
		
		if( ! IsInLoop() )
		{
			mLogger.PushError(n->coords, "break can only be used inside a loop");
			return false;
		}
		
		if( n->value )
			return AnalyzeNode(n->value);
		
		return true;
	}
	
	case ast::Node::N_Continue:
	{
		ast::ContinueNode* n = (ast::ContinueNode*)node;
		
		if( IsInConstruction() )
		{
			mLogger.PushError(n->coords, "continue cannot be used inside array, object or argument constructions");
			return false;
		}
		
		if( ! IsInLoop() )
		{
			mLogger.PushError(n->coords, "continue can only be used inside a loop");
			return false;
		}
		
		if( n->value )
			return AnalyzeNode(n->value);
		
		return true;
	}
	
	default:
		{}
	}
	
	return true;
}

bool SemanticAnalyzer::IsBreakContinueReturn(const ast::Node* node) const
{
	return	node->type == ast::Node::N_Break ||
			node->type == ast::Node::N_Continue ||
			node->type == ast::Node::N_Return;
}

bool SemanticAnalyzer::IsBreakContinue(const ast::Node* node) const
{
	return	node->type == ast::Node::N_Break ||
			node->type == ast::Node::N_Continue;
}

bool SemanticAnalyzer::IsReturn(const ast::Node* node) const
{
	return	node->type == ast::Node::N_Return;
}

bool SemanticAnalyzer::IsInLoop() const
{
	for( auto rit = mContext.rbegin(); rit != mContext.rend(); ++rit )
	{
		if( *rit == CXT_InFunction )
			return false;
		if( *rit == CXT_InLoop )
			return true;
	}
	
	return false;
}

bool SemanticAnalyzer::IsInFunction() const
{
	for( auto rit = mContext.rbegin(); rit != mContext.rend(); ++rit )
		if( *rit == CXT_InFunction )
			return true;
	
	return false;
}

bool SemanticAnalyzer::IsInConstruction() const
{
	return	mContext.back() == CXT_InArray ||
			mContext.back() == CXT_InObject ||
			mContext.back() == CXT_InArguments;
}

}
