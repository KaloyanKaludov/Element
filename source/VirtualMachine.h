#ifndef _VIRTUAL_MACHINE_INCLUDED_
#define _VIRTUAL_MACHINE_INCLUDED_

#include <deque>
#include <unordered_map>

#include "Logger.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "Compiler.h"
#include "FileManager.h"
#include "MemoryManager.h"

namespace element
{

class VirtualMachine
{
public:				VirtualMachine();
	
	void			ResetState();

	Value			Interpret(std::istream& input);
	Value			Interpret(const std::string& filename);
	
	FileManager&	GetFileManager();
	MemoryManager&	GetMemoryManager();

	void			SetError(const std::string& errorMessage);
	bool			HasError() const;
	void			ClearError();
	
	void			RegisterNativeFunction(const std::string& name, Value::NativeFunction function);
	std::string		GetVersion() const;
	
	// value manipulation //////////////////////////////////////////////////////
	Iterator*		MakeIteratorForValue(const Value& value);
	unsigned		GetHashFromName(const std::string& name);
	bool			GetNameFromHash(unsigned hash, std::string* name);

	Value			GetMember(const Value& object, const std::string& memberName);
	Value			GetMember(const Value& object, unsigned memberHash) const;

	void			SetMember(const Value& object, const std::string& memberName, const Value& value);
	void			SetMember(const Value& object, unsigned memberHash, const Value& value);
	
	void			PushElement(const Value& array, const Value& value);
	void			AddElement(const Value& array, int atIndex, const Value& value);
	
	Value			CallFunction(const Value& function, const std::vector<Value>& args);

	Value			CallMemberFunction(const Value& object, const std::string& memberFunctionName, const std::vector<Value>& args);
	Value			CallMemberFunction(const Value& object, unsigned functionHash, const std::vector<Value>& args);
	Value			CallMemberFunction(const Value& object, const Value& function, const std::vector<Value>& args);

protected:
	Value			ExecuteBytecode(const char* bytecode, Module& forModule);
	int				ParseBytecode(const char* bytecode, Module& forModule);

	Value			CallFunction_Common(const Value& thisObject, const Value& function, const std::vector<Value>& args);

	Value			RunCode();
	void			RunCodeForFrame(StackFrame* frame);

	void			Call(int argumentsCount);
	void			CallNative(int argumentsCount);

	void			PushElementToArray(Array* array, const Value& newValue);
	bool			PopElementFromArray(Array* array, Value* outValue);
	void			LoadElementFromArray(const Array* array, int index, Value* outValue);
	void			StoreElementInArray(Array* array, int index, const Value& newValue);
	
	void			LoadMemberFromObject(Object* object, unsigned hash, Value* outValue) const;
	void			StoreMemberInObject(Object* object, unsigned hash, const Value& newValue);

	bool			DoBinaryOperation(int opCode);

	void			RegisterStandardUtilities();
	
	void			LogStackTraceStartingFrom(const StackFrame* frame);
	void			LocationFromFrame(const StackFrame* frame, int* currentLine, std::string* currentFile) const;

private:
	Logger										mLogger;
	Parser										mParser;
	SemanticAnalyzer							mSemanticAnalyzer;
	Compiler									mCompiler;
	FileManager									mFileManager;
	MemoryManager								mMemoryManager;

	std::vector<Value>							mConstants; // this is the common access point for the 3 deques below
	std::deque<String>							mConstantStrings;
	std::deque<Function>						mConstantFunctions;
	std::deque<CodeObject>						mConstantCodeObjects;

	std::vector<Value::NativeFunction>			mNativeFunctions;
	std::unordered_map<unsigned, std::string>	mSymbolNames;

	ExecutionContext*							mExecutionContext;
	std::vector<Value>*							mStack;
	
	std::string									mErrorMessage;
};

}

#endif // _VIRTUAL_MACHINE_INCLUDED_
