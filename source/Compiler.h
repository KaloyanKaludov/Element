#ifndef _COMPILER_INCLUDED_
#define _COMPILER_INCLUDED_

#include <memory>
#include <vector>
#include <deque>
#include <unordered_map>
#include "Constant.h"
#include "Symbol.h"

namespace element
{

class Logger;
struct CodeObject;

namespace ast
{
	struct Node;
	struct FunctionNode;
}


class Compiler
{
public:  Compiler(Logger& logger);

	auto Compile(const ast::FunctionNode* node) -> std::unique_ptr<char[]>;

	void ResetState();

protected:
	struct FunctionContext
	{
		std::vector<unsigned>	jumpToEndIndices;
		int						index = -1;
		int						totalIndices = 0;
		int						forLoopsGarbage = 0;
	};

	struct LoopContext
	{
		std::vector<unsigned>	jumpToConditionIndices;
		std::vector<unsigned>	jumpToEndIndices;
		bool					keepValue;
		bool					forLoop;
	};

protected:
	void EmitInstructions	(const ast::Node* node, bool keepValue);

	void BuildConstantLoad	(const ast::Node* node, bool keepValue);
	void BuildVariableLoad	(const ast::Node* node, bool keepValue);
	void BuildVariableStore	(const ast::Node* node, bool keepValue);
	void BuildAssignOp		(const ast::Node* node, bool keepValue);
	void BuildBooleanOp		(const ast::Node* node, bool keepValue);
	void BuildArrowOp		(const ast::Node* node, bool keepValue);
	void BuildArrayPushPop	(const ast::Node* node, bool keepValue);
	void BuildBinaryOp		(const ast::Node* node, bool keepValue);
	void BuildUnaryOp		(const ast::Node* node, bool keepValue);
	void BuildIf			(const ast::Node* node, bool keepValue);
	void BuildWhile			(const ast::Node* node, bool keepValue);
	void BuildFor			(const ast::Node* node, bool keepValue);
	void BuildBlock			(const ast::Node* node, bool keepValue);
	void BuildFunction		(const ast::Node* node, bool keepValue);
	void BuildFunctionCall	(const ast::Node* node, bool keepValue);
	void BuildArray			(const ast::Node* node, bool keepValue);
	void BuildObject		(const ast::Node* node, bool keepValue);
	void BuildYield			(const ast::Node* node, bool keepValue);
	bool BuildHashLoadOp	(const ast::Node* node);
	void BuildJumpStatement	(const ast::Node* node);

	unsigned UpdateSymbol(const std::string& name);

	std::unique_ptr<char[]> BuildBinaryData();

private:
	Logger&									mLogger;

	std::vector<LoopContext>				mLoopContexts;
	std::vector<FunctionContext>			mFunctionContexts;

	CodeObject*								mCurrentFunction;
	
	std::deque<Constant>					mConstants;
	unsigned								mConstantsOffset;

	std::unordered_map<unsigned, unsigned>	mSymbolIndices;
	std::vector<Symbol>						mSymbols;
	unsigned								mSymbolsOffset;
};

}

#endif // _COMPILER_INCLUDED_
