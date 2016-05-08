#include "Interpreter.h"

#include "AST.h"
#include "Native.h"

namespace element
{

Interpreter::Interpreter()
: mLogger			()
, mLexer			(mLogger)
, mParser			(mLexer, mLogger)
, mSemanticAnalyzer	(mLogger)
, mCompiler			(mLogger)
, mVirtualMachine	(mLogger)

, mDebugPrintAst(false)
, mDebugPrintSymbols(false)
, mDebugPrintConstants(false)
{
	RegisterNativeFunction("type",				nativefunctions::Type);
	RegisterNativeFunction("this_call",			nativefunctions::ThisCall);
	RegisterNativeFunction("garbage_collect",	nativefunctions::GarbageCollect);
	RegisterNativeFunction("memory_stats",		nativefunctions::MemoryStats);
	RegisterNativeFunction("print",				nativefunctions::Print);
	RegisterNativeFunction("to_upper",			nativefunctions::ToUpper);
	RegisterNativeFunction("to_lower",			nativefunctions::ToLower);
	RegisterNativeFunction("make_generator",	nativefunctions::MakeGenerator);
	RegisterNativeFunction("range",				nativefunctions::Range);
	RegisterNativeFunction("each",				nativefunctions::Each);
	RegisterNativeFunction("times",				nativefunctions::Times);
	RegisterNativeFunction("count",				nativefunctions::Count);
	RegisterNativeFunction("map",				nativefunctions::Map);
	RegisterNativeFunction("filter",			nativefunctions::Filter);
	RegisterNativeFunction("reduce",			nativefunctions::Reduce);
	RegisterNativeFunction("all",				nativefunctions::All);
	RegisterNativeFunction("any",				nativefunctions::Any);
	RegisterNativeFunction("min",				nativefunctions::Min);
	RegisterNativeFunction("max",				nativefunctions::Max);
	RegisterNativeFunction("sort",				nativefunctions::Sort);
	RegisterNativeFunction("abs",				nativefunctions::Abs);
	RegisterNativeFunction("floor",				nativefunctions::Floor);
	RegisterNativeFunction("ceil",				nativefunctions::Ceil);
	RegisterNativeFunction("round",				nativefunctions::Round);
	RegisterNativeFunction("sqrt",				nativefunctions::Sqrt);
	RegisterNativeFunction("sin",				nativefunctions::Sin);
	RegisterNativeFunction("cos",				nativefunctions::Cos);
	RegisterNativeFunction("tan",				nativefunctions::Tan);
}

void Interpreter::Interpret(std::istream& input)
{
	std::vector<ast::Node*> expressions;
	ast::Node* node = nullptr;
	
	char* bytecode = nullptr;
	
	mLexer.SetInputStream(input);
	mLexer.GetNextToken();
	
	// Parsing stage ///////////////////////////////////////////////////////////
	node = mParser.ParseExpression();
	
	while( node )
	{
		expressions.push_back(node);
		node = mParser.ParseExpression();
	}
	
	if( mDebugPrintAst )
		for( const auto& expression : expressions )
			mParser.DebugPrintAST(expression);
	
	if( mLogger.HasErrorMessages() )
	{
		mLogger.PrintErrorMessages();
		mLogger.ClearErrorMessages();
		goto clean;
	}
	
	// Semantic Analysis stage /////////////////////////////////////////////////
	mSemanticAnalyzer.Analyze(expressions);
	
	if( mLogger.HasErrorMessages() )
	{
		mLogger.PrintErrorMessages();
		mLogger.ClearErrorMessages();
		goto clean;
	}
	
	// Compilation stage ///////////////////////////////////////////////////////
	bytecode = mCompiler.Compile(expressions);
	
	if( mDebugPrintSymbols || mDebugPrintConstants )
		mCompiler.DebugPrintBytecode(bytecode, mDebugPrintSymbols, mDebugPrintConstants);
	
	if( mLogger.HasErrorMessages() )
	{
		mLogger.PrintErrorMessages();
		mLogger.ClearErrorMessages();
		goto clean;
	}
	
	// Execution stage /////////////////////////////////////////////////////////
	mVirtualMachine.Execute(bytecode);
	
	if( mLogger.HasErrorMessages() )
	{
		mLogger.PrintErrorMessages();
		mLogger.ClearErrorMessages();
		goto clean;
	}
	
	// When all is said and done, time to take out the trash ///////////////////
clean:
	mVirtualMachine.ClearError();

	if( bytecode )
		delete bytecode;
	
	for( const auto& expression : expressions )
		delete expression;
}

void Interpreter::Interpret(const char* bytecode)
{
	mVirtualMachine.Execute(bytecode);

	if( mLogger.HasErrorMessages() )
	{
		mLogger.PrintErrorMessages();
		mLogger.ClearErrorMessages();
	}

	mVirtualMachine.ClearError();
}

void Interpreter::SetDebugPrintAst(bool state)
{
	mDebugPrintAst = state;
}

void Interpreter::SetDebugPrintSymbols(bool state)
{
	mDebugPrintSymbols = state;
}

void Interpreter::SetDebugPrintConstants(bool state)
{
	mDebugPrintConstants = state;
}

std::string Interpreter::GetVersion() const
{
	return "element interpreter version 0.0.1";
}

void Interpreter::RegisterNativeFunction(const std::string& name, Value::NativeFunction function)
{
	int index = mVirtualMachine.AddNativeFunction(function);
	mCompiler.AddNativeFunction(name, index);
}

}
