#include "SemanticAnalyzer.h"

#include <algorithm>
#include "Logger.h"
#include "AST.h"

namespace element
{

SemanticAnalyzer::FunctionScope::FunctionScope(ast::FunctionNode* n)
	: node(n)
{
	blocks.emplace_back();

	parameters = n->namedParameters;

	n->localVariablesCount = int(parameters.size());
}


SemanticAnalyzer::SemanticAnalyzer(Logger& logger)
: mLogger(logger)
, mCurrentFunctionNode(nullptr)
{
}

void SemanticAnalyzer::Analyze(ast::FunctionNode* node)
{
	AnalyzeNode(node);

	ResolveNamesInNodes({node});

	mContext.clear();
	mFunctionScopes.clear();
}

void SemanticAnalyzer::AddNativeFunction(const std::string& name, int index)
{
	mNativeFunctions[name] = index;
}

void SemanticAnalyzer::ResetState()
{
	mContext.clear();
	mFunctionScopes.clear();
	mGlobalVariables.clear();
	mNativeFunctions.clear();

	mCurrentFunctionNode = nullptr;
}

void SemanticAnalyzer::DebugPrintSemantics(const ast::Node* root, int indent) const
{
	// TODO
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
		return true;

	case ast::Node::N_Variable:
		mCurrentFunctionNode->referencedVariables.push_back((ast::VariableNode*)node);
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
		return AnalyzeBinaryOperator((ast::BinaryOperatorNode*)node);

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

		if( ! CheckAssignable(n->iterator) )
			return false;

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
		{
            if( it.first->type != ast::Node::N_Variable )
			{
				mLogger.PushError(it.first->coords, "only valid identifiers can be object keys");
				return false;
			}

			if( static_cast<ast::VariableNode*>(it.first)->variableType != ast::VariableNode::V_Named )
			{
				mLogger.PushError(it.first->coords, "only valid identifiers can be object keys");
				return false;
			}

			if( ! AnalyzeNode(it.second) )
				return false;
		}

		mContext.pop_back();

		return true;
	}

	case ast::Node::N_Function:
	{
		ast::FunctionNode* n = (ast::FunctionNode*)node;

		if( n->body->type == ast::Node::N_Block )
			static_cast<ast::BlockNode*>(n->body)->explicitFunctionBlock = true;

		mContext.push_back(mContext.empty() ? ContextType::CXT_InGlobal : ContextType::CXT_InFunction);

		ast::FunctionNode* oldFunctionNode = mCurrentFunctionNode;
		mCurrentFunctionNode = n;

		bool ok = AnalyzeNode(n->body);

		mCurrentFunctionNode = oldFunctionNode;

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
			nodeType == ast::Node::N_Continue	||
			nodeType == ast::Node::N_Yield )
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

	case ast::Node::N_Yield:
	{
		ast::YieldNode* n = (ast::YieldNode*)node;

		if( IsInConstruction() )
		{
			mLogger.PushError(n->coords, "yield cannot be used inside array, object or argument constructions");
			return false;
		}

		if( ! IsInFunction() )
		{
			mLogger.PushError(n->coords, "yield can only be used inside a function");
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

bool SemanticAnalyzer::AnalyzeBinaryOperator(const ast::BinaryOperatorNode* n)
{
	if( n->op != Token::T_And &&
		n->op != Token::T_Or &&
		(IsBreakContinueReturn(n->lhs) || IsBreakContinueReturn(n->rhs)) )
	{
		mLogger.PushError(n->coords, "break, continue, return can only be used with 'and' and 'or' operators");
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
		if( lhsType == ast::Node::N_Array && n->op != Token::T_Assignment )
		{
			mLogger.PushError(n->lhs->coords, "cannot use compound assignment with an array on the left hand side");
			return false;
		}

		if( ! CheckAssignable(n->lhs) )
			return false;
	}

	if( n->op == Token::T_Assignment &&
		n->rhs->type == ast::Node::N_Variable &&
		((ast::VariableNode*)n->rhs)->variableType == ast::VariableNode::V_ArgumentList )
	{
		mLogger.PushError(n->rhs->coords, "argument arrays cannot be assigned to variables, they must be copied");
		return false;
	}

	if( n->op == Token::T_ArrayPopBack )
	{
		if( ! CheckAssignable(n->rhs) )
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
			lhsType == ast::Node::N_Continue	||
			lhsType == ast::Node::N_Yield )
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
			lhsType == ast::Node::N_Continue	||
			lhsType == ast::Node::N_Yield )
		{
			mLogger.PushError(n->lhs->coords, "invalid target for member access");
			return false;
		}
	}

	return AnalyzeNode(n->lhs) && AnalyzeNode(n->rhs);
}

bool SemanticAnalyzer::CheckAssignable(const ast::Node* node) const
{
	if( node->type == ast::Node::N_Variable )
	{
		const ast::VariableNode* variable = (const ast::VariableNode*)node;

		if( variable->variableType == ast::VariableNode::V_This )
		{
			mLogger.PushError(variable->coords, "the 'this' variable is not assignable");
			return false;
		}
		if( variable->variableType == ast::VariableNode::V_ArgumentList )
		{
			mLogger.PushError(variable->coords, "the $$ array is not assignable");
			return false;
		}
		if( variable->variableType >= 0 ) // anonymous parameter
		{
			mLogger.PushError(variable->coords, "the $%d variable is not assignable", variable->variableType);
			return false;
		}

		return true;
	}
	else if( node->type == ast::Node::N_BinaryOperator )
	{
		const ast::BinaryOperatorNode* bin = (const ast::BinaryOperatorNode*)node;

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

		if( bin->op == Token::T_Dot )
			return CheckAssignable( bin->rhs );

		return true;
	}
	else if( node->type == ast::Node::N_Array )
	{
		const ast::ArrayNode* array = (const ast::ArrayNode*)node;

		for( const ast::Node* e : array->elements )
			if( ! CheckAssignable(e) )
				return false;

		return true;
	}

	// nothing else is assignable
	mLogger.PushError(node->coords, "invalid assignment");
	return false;
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

void SemanticAnalyzer::ResolveNamesInNodes(std::vector<ast::Node*> nodesToProcess)
{
	std::vector<ast::Node*> nodesToDefer;
	ast::Node* node = nullptr;

	while( !nodesToProcess.empty() )
	{
		node = nodesToProcess.back();

		nodesToProcess.pop_back();

		switch( node->type )
		{
		case ast::Node::N_Nil:
		case ast::Node::N_Bool:
		case ast::Node::N_Integer:
		case ast::Node::N_Float:
		case ast::Node::N_String:
		default:
			break;

		case ast::Node::N_Variable:
		{
			ast::VariableNode* vn = (ast::VariableNode*)node;
			if( vn->variableType == ast::VariableNode::V_Named )
				ResolveName(vn);
			break;
		}

		case ast::Node::N_Block:
		case ast::Node::N_Function:
			nodesToDefer.push_back(node);
			break;

		case ast::Node::N_Arguments:
		{
			ast::ArgumentsNode* n = (ast::ArgumentsNode*)node;
			for( auto it = n->arguments.rbegin(); it != n->arguments.rend(); ++it )
				nodesToProcess.push_back(*it);
			break;
		}

		case ast::Node::N_UnaryOperator:
		{
			ast::UnaryOperatorNode* n = (ast::UnaryOperatorNode*)node;
			nodesToProcess.push_back(n->operand);
			break;
		}

		case ast::Node::N_BinaryOperator:
		{
			ast::BinaryOperatorNode* n = (ast::BinaryOperatorNode*)node;
			nodesToProcess.push_back(n->rhs);
			nodesToProcess.push_back(n->lhs);
			break;
		}

		case ast::Node::N_If:
		{
			ast::IfNode* n = (ast::IfNode*)node;
			if( n->elsePath )
				nodesToProcess.push_back(n->elsePath);
			nodesToProcess.push_back(n->thenPath);
			nodesToProcess.push_back(n->condition);
			break;
		}

		case ast::Node::N_While:
		{
			ast::WhileNode* n = (ast::WhileNode*)node;
			nodesToProcess.push_back(n->body);
			nodesToProcess.push_back(n->condition);
			break;
		}

		case ast::Node::N_For:
		{
			ast::ForNode* n = (ast::ForNode*)node;
			nodesToProcess.push_back(n->body);
			nodesToProcess.push_back(n->iteratedExpression);
			nodesToProcess.push_back(n->iterator);
			break;
		}

		case ast::Node::N_Array:
		{
			ast::ArrayNode* n = (ast::ArrayNode*)node;
			for( auto it = n->elements.rbegin(); it != n->elements.rend(); ++it )
				nodesToProcess.push_back(*it);
			break;
		}

		case ast::Node::N_Object:
		{
			ast::ObjectNode* n = (ast::ObjectNode*)node;
			for( auto it = n->members.rbegin(); it != n->members.rend(); ++it )
			{
				nodesToProcess.push_back(it->second);
				nodesToProcess.push_back(it->first);
			}
			break;
		}

		case ast::Node::N_FunctionCall:
		{
			ast::FunctionCallNode* n = (ast::FunctionCallNode*)node;
			nodesToProcess.push_back(n->arguments);
			nodesToProcess.push_back(n->function);
			break;
		}

		case ast::Node::N_Return:
		{
			ast::ReturnNode* n = (ast::ReturnNode*)node;
			if( n->value )
				nodesToProcess.push_back(n->value);
			break;
		}

		case ast::Node::N_Break:
		{
			ast::BreakNode* n = (ast::BreakNode*)node;
			if( n->value )
				nodesToProcess.push_back(n->value);
			break;
		}

		case ast::Node::N_Continue:
		{
			ast::ContinueNode* n = (ast::ContinueNode*)node;
			if( n->value )
				nodesToProcess.push_back(n->value);
			break;
		}

		case ast::Node::N_Yield:
		{
			ast::YieldNode* n = (ast::YieldNode*)node;
			if( n->value )
				nodesToProcess.push_back(n->value);
			break;
		}
		}
	}

	for( ast::Node* deferredNode : nodesToDefer )
	{
		switch( deferredNode->type )
		{
		default:
		{}

		case ast::Node::N_Block:
		{
			ast::BlockNode* n = (ast::BlockNode*)deferredNode;

			std::vector<ast::Node*> toProcess(n->nodes.rbegin(), n->nodes.rend());

			if( n->explicitFunctionBlock )
			{
				ResolveNamesInNodes(toProcess);
			}
			else // regular block
			{
				mFunctionScopes.back().blocks.emplace_back();

				ResolveNamesInNodes(toProcess);

				mFunctionScopes.back().blocks.pop_back();
			}

			break;
		}

		case ast::Node::N_Function:
		{
			ast::FunctionNode* n = (ast::FunctionNode*)deferredNode;

			bool isGlobal = mFunctionScopes.empty();

			mFunctionScopes.emplace_back( n );

			if( isGlobal )
				mFunctionScopes.back().blocks.pop_back();

			ResolveNamesInNodes({n->body});

			mFunctionScopes.pop_back();

			break;
		}
		}
	}
}

void SemanticAnalyzer::ResolveName(ast::VariableNode* vn)
{
	const std::string& name = vn->name;

	// if this is the global function scope
	if( mFunctionScopes.size() == 1 && mFunctionScopes.front().blocks.empty() )
	{
		// try the global scope
		auto globalIt = std::find(mGlobalVariables.begin(), mGlobalVariables.end(), name);

		if( globalIt != mGlobalVariables.end() )
		{
			vn->semanticType	= ast::VariableNode::SMT_Global;
			vn->index			= std::distance(mGlobalVariables.begin(), globalIt);
			vn->firstOccurrence	= false;
			return;
		}

		// try the native constants
		auto nativeIt = mNativeFunctions.find(name);

		if( nativeIt != mNativeFunctions.end() )
		{
			vn->semanticType	= ast::VariableNode::SMT_Native;
			vn->index			= nativeIt->second;
			vn->firstOccurrence	= false;
			return;
		}

		// otherwise create a new global
		vn->semanticType	= ast::VariableNode::SMT_Global;
		vn->index			= int(mGlobalVariables.size());
		vn->firstOccurrence	= true;

		mGlobalVariables.push_back(name);
		return;
	}

	// otherwise we are inside a function
	FunctionScope& localFunctionScope = mFunctionScopes.back();

	// try the parameters of this function
	int parametersSize = int(localFunctionScope.parameters.size());
	for( int i = 0; i < parametersSize; ++i )
	{
		if( name == localFunctionScope.parameters[i] )
		{
			vn->semanticType	= ast::VariableNode::SMT_Local;
			vn->index			= i;
			vn->firstOccurrence = false;
			return;
		}
	}

	// try the free variables captured by this function
	int freeVariablesSize = int(localFunctionScope.freeVariables.size());
	for( int i = 0; i < freeVariablesSize; ++i )
	{
		if( name == localFunctionScope.freeVariables[i] )
		{
			vn->semanticType	= ast::VariableNode::SMT_FreeVariable;
			vn->index			= i;
			vn->firstOccurrence = false;
			return;
		}
	}

	// try the blocks in the current function scope in reverse
	for( auto blockIt = localFunctionScope.blocks.rbegin(); blockIt != localFunctionScope.blocks.rend(); ++blockIt )
	{
		auto localNameIt = blockIt->variables.find(name);

		if( localNameIt != blockIt->variables.end() )
		{
			vn->semanticType	= localNameIt->second->semanticType;
			vn->index			= localNameIt->second->index;
			vn->firstOccurrence	= false;
			return;
		}
	}

	// try the enclosing function scopes if this is part of a closure
	if( TryToFindNameInTheEnclosingFunctions( vn ) )
		return;

	// try the global scope (check this after the parameters, because they can hide globals)
	auto globalIt = std::find(mGlobalVariables.begin(), mGlobalVariables.end(), name);

	if( globalIt != mGlobalVariables.end() )
	{
		vn->semanticType	= ast::VariableNode::SMT_Global;
		vn->index			= std::distance(mGlobalVariables.begin(), globalIt);
		vn->firstOccurrence	= false;
		return;
	}

	// try the native constants (check this after the parameters, because they can hide natives)
	auto nativeIt = mNativeFunctions.find(name);

	if( nativeIt != mNativeFunctions.end() )
	{
		vn->semanticType	= ast::VariableNode::SMT_Native;
		vn->index			= nativeIt->second;
		vn->firstOccurrence	= false;
		return;
	}

	// we didn't find it anywhere, create it locally
	vn->semanticType	= ast::VariableNode::SMT_Local;
	vn->index			= localFunctionScope.node->localVariablesCount++;
	vn->firstOccurrence	= true;

	localFunctionScope.blocks.back().variables[name] = vn;
}

bool SemanticAnalyzer::TryToFindNameInTheEnclosingFunctions(ast::VariableNode* vn)
{
	bool found = false;
	int foundAtIndex = 0;
	const std::string& name = vn->name;

	const auto makeBoxed = [](FunctionScope& fs, int index)
	{
		for( ast::VariableNode* vn : fs.node->referencedVariables )
		{
			if( vn->index == index )
				vn->semanticType = ast::VariableNode::SMT_LocalBoxed;
		}

		if( index < int(fs.node->namedParameters.size()) )
		{
			fs.node->parametersToBox.emplace_back( index );
		}
	};

	// try each of the enclosing function scopes in reverse
	for( auto functionIt = ++mFunctionScopes.rbegin(); functionIt != mFunctionScopes.rend(); ++functionIt )
	{
		int freeVariablesSize = int(functionIt->freeVariables.size());
		for( int i = 0; i < freeVariablesSize; ++i )
		{
			if( name == functionIt->freeVariables[i] )
			{
				found = true;
				foundAtIndex = -i - 1; // negative index for free variables
				break;
			}
		}

		if( !found )
		{
			int parametersSize = int(functionIt->parameters.size());
			for( int i = 0; i < parametersSize; ++i )
			{
				if( name == functionIt->parameters[i] )
				{
					found = true;
					foundAtIndex = i;

					// If this is the first time this parameter has ever been captured,
					// all references to it in this function scope must become 'SMT_LocalBoxed'.
					const auto& ptb = functionIt->node->parametersToBox;
					if( std::find(ptb.begin(), ptb.end(), i) == ptb.end() )
					{
						makeBoxed(*functionIt, foundAtIndex);
					}
					break;
				}
			}
		}

		if( !found )
		{
			std::vector<BlockScope>& blocks = functionIt->blocks;

			for( auto blockIt = blocks.rbegin(); blockIt != blocks.rend(); ++blockIt )
			{
				auto localNameIt = blockIt->variables.find(name);

				if( localNameIt != blockIt->variables.end() )
				{
					found = true;
					foundAtIndex = localNameIt->second->index;

					// If this is the first time this variable has ever been captured,
					// all references to it in this function scope must become 'SMT_LocalBoxed'.
					if( localNameIt->second->semanticType == ast::VariableNode::SMT_Local )
					{
						makeBoxed(*functionIt, foundAtIndex);
					}
					break;
				}
			}
		}

		if( found )
		{
			int foundFunctionScopeIndex = std::distance(mFunctionScopes.begin(), functionIt.base()) - 1;
			int localFunctionScopeIndex = int(mFunctionScopes.size() - 1);

			while( foundFunctionScopeIndex + 1 < localFunctionScopeIndex )
			{
				FunctionScope& functionScope = mFunctionScopes[foundFunctionScopeIndex + 1];

				int newFreeVarIndex = int(functionScope.freeVariables.size());
				newFreeVarIndex = -newFreeVarIndex - 1; // negative index for free variables

				functionScope.freeVariables.push_back( name );
				functionScope.node->closureMapping.push_back( foundAtIndex );

				foundAtIndex = newFreeVarIndex;

				++foundFunctionScopeIndex;
			}

			FunctionScope& localFunctionScope = mFunctionScopes.back();

			localFunctionScope.freeVariables.push_back( name );
			localFunctionScope.node->closureMapping.push_back( foundAtIndex );

			vn->semanticType	= ast::VariableNode::SMT_FreeVariable;
			vn->index			= int(localFunctionScope.freeVariables.size()) - 1;
			vn->firstOccurrence = true;

			return true;
		}
	}

	return false;
}

}
