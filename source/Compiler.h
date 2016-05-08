#ifndef _COMPILER_INCLUDED_
#define _COMPILER_INCLUDED_

#include <vector>
#include <deque>
#include <map>
#include "AST.h"
#include "Constant.h"
#include "Symbol.h"
#include "OpCodes.h"
#include "DataTypes.h"

namespace element
{

class Logger;


class Compiler
{
public:		Compiler(Logger& logger);
	char*	Compile(const std::vector<ast::Node*>& astNodes);
	void	ResetState();
	void	DebugPrintBytecode(const char* bytecode, bool printSymbols, bool printConstants) const;
	
	void	AddNativeFunction(const std::string& name, int index);

protected:
	enum NameType
	{
		NT_Global,
		NT_Local,
		NT_Boxed,
		NT_Native,
	};
	
	typedef std::map<std::string, int> BlockScope;

	struct FunctionContext
	{
		std::vector<unsigned>		jumpToEndIndices;
		
		std::vector<AskedVariable>	askedVariables;
		std::vector<unsigned>		localIndicesToBox;
		
		std::deque<BlockScope>		blockScopes;
		int							totalIndices = 0;
		
		int							forLoopsGarbage = 0;
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
	void BuildElementStore	(const ast::Node* node, bool keepValue);
	void BuildMemberStore	(const ast::Node* node, bool keepValue);
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
	bool BuildHashLoadOp	(const ast::Node* node);
	void BuildJumpStatement	(const ast::Node* node);
	
	void PushFunctionContext(const ast::FunctionNode* n);
	int  PopFunctionContext(int endLocation);
	
	void PushBlockScope();
	void PopBlockScope();
	
	void ResolveName(const std::string& name, NameType* outType, int* outIndex);
	void ResolveBoxed(const std::string& name, int* outIndex);
	
	unsigned UpdateSymbol(const std::string& name, int globalIndex = -1);
	
	char* BuildBinaryData();

private:
	Logger&						mLogger;
	
	std::map<std::string, int>	mNativeFunctions;
	
	std::deque<Constant>		mConstants;
	unsigned					mConstantsTotalOffset;
	
	CodeSegment*				mCurrentFunction;
	std::vector<LoopContext>	mLoopContexts;
	std::vector<FunctionContext>mFunctionContexts;
	
	std::map<std::string, int>	mGlobalScope;
	int							mTotalGlobalIndices;
	
	std::map<unsigned, unsigned>mSymbolIndices;
	std::vector<Symbol>			mSymbols;
	unsigned					mSymbolsOffset;
};

}

#endif // _COMPILER_INCLUDED_
