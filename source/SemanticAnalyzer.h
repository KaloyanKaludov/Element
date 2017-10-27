#ifndef _SEMANTIC_ANALYZER_H_INCLUDED_
#define _SEMANTIC_ANALYZER_H_INCLUDED_

#include <string>
#include <deque>
#include <map>
#include "Logger.h"

namespace element
{

namespace ast
{
	struct Node;
	struct VariableNode;
	struct FunctionNode;
	struct BinaryOperatorNode;
}

class Logger;


class SemanticAnalyzer
{
public:		SemanticAnalyzer(Logger& logger);

	void	Analyze(ast::FunctionNode* node);

	void	AddNativeFunction(const std::string& name, int index);

	void	ResetState();

protected:
	enum ContextType
	{
		CXT_InGlobal,
		CXT_InFunction,
		CXT_InLoop,
		CXT_InObject,
		CXT_InArray,
		CXT_InArguments
	};

	struct BlockScope
	{
		std::map<std::string, ast::VariableNode*> variables;
	};

	struct FunctionScope
	{
		ast::FunctionNode* node;

		std::vector<BlockScope>		blocks;
		std::vector<std::string>	parameters;
		std::vector<std::string>	freeVariables;

		FunctionScope(ast::FunctionNode* n);
	};

protected:
	bool	AnalyzeNode(ast::Node* node);
	bool	AnalyzeBinaryOperator(const ast::BinaryOperatorNode* n);

	bool	CheckAssignable(const ast::Node* node) const;
	bool	IsBreakContinueReturn(const ast::Node* node) const;
	bool	IsBreakContinue(const ast::Node* node) const;
	bool	IsReturn(const ast::Node* node) const;
	bool	IsInLoop() const;
	bool	IsInFunction() const;
	bool	IsInConstruction() const;

	void	ResolveNamesInNodes(std::vector<ast::Node*> nodesToProcess);
	void	ResolveName(ast::VariableNode* vn);
	bool	TryToFindNameInTheEnclosingFunctions(ast::VariableNode* vn);

private:
	Logger&						mLogger;

	std::vector<ContextType>	mContext;

	ast::FunctionNode*			mCurrentFunctionNode;

	std::vector<FunctionScope>	mFunctionScopes;

	std::vector<std::string>	mGlobalVariables;

	std::map<std::string, int>	mNativeFunctions;
};

}

#endif // _SEMANTIC_ANALYZER_H_INCLUDED_
