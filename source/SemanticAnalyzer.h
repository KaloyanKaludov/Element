#ifndef _SEMANTIC_ANALYZER_H_INCLUDED_
#define _SEMANTIC_ANALYZER_H_INCLUDED_

#include "AST.h"

namespace element
{

class Logger;


class SemanticAnalyzer
{
public:		SemanticAnalyzer(Logger& logger);
	void	Analyze(std::vector<ast::Node*>& astNodes);

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
	
protected:
	bool	AnalyzeNode(ast::Node* node);
	
	bool	IsBreakContinueReturn(const ast::Node* node) const;
	bool	IsBreakContinue(const ast::Node* node) const;
	bool	IsReturn(const ast::Node* node) const;
	bool	IsInLoop() const;
	bool	IsInFunction() const;
	bool	IsInConstruction() const;
	
private:
	Logger&	mLogger;
	
	std::vector<ContextType> mContext;
};

}

#endif // _SEMANTIC_ANALYZER_H_INCLUDED_
