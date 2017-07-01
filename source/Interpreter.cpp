#include "Interpreter.h"

#include "AST.h"
#include "Native.h"

namespace element
{

Interpreter::Interpreter()
: mLogger			()
, mParser			(mLogger)
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
	std::unique_ptr<ast::FunctionNode> node = mParser.Parse(input);
	
	if( !mLogger.HasErrorMessages() )
	{
		if( mDebugPrintAst )
			mParser.DebugPrintAST(node.get());
		
		mSemanticAnalyzer.Analyze(node.get());
		
		if( !mLogger.HasErrorMessages() )
		{
			std::unique_ptr<char[]> bytecode = mCompiler.Compile(node.get());
			
			if( !mLogger.HasErrorMessages() )
			{
				if( mDebugPrintSymbols || mDebugPrintConstants )
					mCompiler.DebugPrintBytecode(bytecode.get(), mDebugPrintSymbols, mDebugPrintConstants);
				
				mVirtualMachine.Execute(bytecode.get());
			}
		}
	}
	
	if( mLogger.HasErrorMessages() )
	{
		mLogger.PrintErrorMessages();
		mLogger.ClearErrorMessages();
	}
	
	mVirtualMachine.ClearError();
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
	return "element interpreter version 0.0.2";
}

void Interpreter::RegisterNativeFunction(const std::string& name, Value::NativeFunction function)
{
	int index = mVirtualMachine.AddNativeFunction(function);
	mSemanticAnalyzer.AddNativeFunction(name, index);
}

}
