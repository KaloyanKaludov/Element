#ifndef _INTERPRETER_H_INCLUDED_
#define _INTERPRETER_H_INCLUDED_

#include "Logger.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "Compiler.h"
#include "VirtualMachine.h"

namespace element
{

class Interpreter
{
public:			Interpreter();
	Value		Interpret(std::istream& input);
	Value		Interpret(const char* bytecode);
	
	void		ResetState();

	void		SetDebugPrintAst(bool state);
	void		SetDebugPrintSymbols(bool state);
	void		SetDebugPrintConstants(bool state);
	std::string	GetVersion() const;

	void		GarbageCollect();

	void		RegisterNativeFunction(const std::string& name, Value::NativeFunction function);

protected:
	void		RegisterStandardNativeFunctions();

private:
	Logger				mLogger;
	Parser				mParser;
	SemanticAnalyzer	mSemanticAnalyzer;
	Compiler			mCompiler;
	VirtualMachine		mVirtualMachine;

	bool				mDebugPrintAst;
	bool				mDebugPrintSymbols;
	bool				mDebugPrintConstants;
};

}

#endif // _INTERPRETER_H_INCLUDED_
