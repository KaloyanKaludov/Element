#ifndef _VIRTUAL_MACHINE_INCLUDED_
#define _VIRTUAL_MACHINE_INCLUDED_

#include <deque>
#include <map>

#include "MemoryManager.h"

namespace element
{

class Logger;


class VirtualMachine
{
public:				VirtualMachine(Logger& logger);

	Value			Execute(const char* bytecode);
	int				AddNativeFunction(Value::NativeFunction function);
	MemoryManager&	GetMemoryManager();

	void			SetError(const char* format, ...);
	bool			HasError() const;
	void			ClearError();

	unsigned		GetHashFromName(const std::string& name);
	int				GetGlobalIndex(const std::string& name) const;
	Value			GetGlobal(int index) const;

	Value			GetMember(Value& object, const std::string& memberName);
	Value			GetMember(Value& object, unsigned memberHash) const;
	void			SetMember(Value& object, unsigned memberHash, Value&& value);

	Value			CallFunction(const std::string& name, const std::vector<Value>& args);
	Value			CallFunction(int index, const std::vector<Value>& args);
	Value			CallFunction(const Value& function, const std::vector<Value>& args);

	Value			CallMemberFunction(Value& object, const std::string& memberFunctionName, const std::vector<Value>& args);
	Value			CallMemberFunction(Value& object, unsigned functionHash, const std::vector<Value>& args);
	Value			CallMemberFunction(const Value& object, const Value& function, const std::vector<Value>& args);

protected:
	struct SymbolInfo
	{
		std::string name;
		int globalIndex;
	};

protected:
	Value	CallFunction_Common(Object* thisObject, const Value& function, const std::vector<Value>& args);

	int		ParseBytecode(const char* bytecode);

	Value	RunCode();
	void	RunCodeForFrame(StackFrame* frame);

	void	Call(int argumentsCount);
	void	CallNative(int argumentsCount);

	void	LoadElementFromArray(const Array* array, int index, Value* outValue) const;
	void	StoreElementInArray(Array* array, int index, Value& newValue);
	void	LoadMemberFromObject(Object* object, unsigned hash, Value* outValue) const;
	void	StoreMemberInObject(Object* object, unsigned hash, Value& newValue);

	bool	DoBinaryOperation(int opCode);

	int		CurrentLineFromFrame(const StackFrame* frame) const;

private:
	std::string							mErrorMessage;
	Logger&								mLogger;

	MemoryManager						mMemoryManager;

	std::vector<Value>					mConstants; // this is the common access point for the 3 deques below
	std::deque<String>					mConstantStrings;
	std::deque<Function>				mConstantFunctions;
	std::deque<CodeObject>				mConstantCodeObjects;

	std::vector<Value::NativeFunction>	mNativeFunctions;
	std::map<unsigned, SymbolInfo>		mSymbolInfos;

	ExecutionContext*					mExecutionContext;
	std::vector<Value>*					mStack;
	std::vector<Value>&					mGlobals;
};

}

#endif // _VIRTUAL_MACHINE_INCLUDED_
