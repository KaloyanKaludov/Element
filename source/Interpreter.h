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
	void		Interpret(std::istream& input);
	void		Interpret(const char* bytecode);

	void		SetDebugPrintAst(bool state);
	void		SetDebugPrintSymbols(bool state);
	void		SetDebugPrintConstants(bool state);
	std::string	GetVersion() const;
	
	void		RegisterNativeFunction(const std::string& name, Value::NativeFunction function);

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
