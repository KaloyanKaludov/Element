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
	
	void			Execute(const char* bytecode);
	int				AddNativeFunction(Value::NativeFunction function);
	MemoryManager&	GetMemoryManager();
	void			GarbageCollect(int steps = std::numeric_limits<int>::max());

	void			SetError(const char* format, ...);
	void			PropagateError();
	bool			HasError() const;
	void			ClearError();
	
	unsigned		GetHashFromName(const std::string& name);
	int				GetGlobalIndex(const std::string& name) const;
	Value			GetGlobal(int index) const;
	
	Value			GetMember(Value& object, const std::string& memberName);
	Value			GetMember(Value& object, unsigned memberHash) const;
	void			SetMember(Value& object, unsigned memberHash, const Value& value);
	
	Value			CallFunction(const std::string& name, const std::vector<Value>& args);
	Value			CallFunction(int index, const std::vector<Value>& args);
	Value			CallFunction(const Value& function, const std::vector<Value>& args);
	
	Value			CallMemberFunction(Value& object, const std::string& memberFunctionName, const std::vector<Value>& args);
	Value			CallMemberFunction(Value& object, unsigned functionHash, const std::vector<Value>& args);
	Value			CallMemberFunction(const Value& object, const Value& function, const std::vector<Value>& args);
	
protected:
	enum ErrorState : char
	{
		ES_NoError		= 0,
		ES_Error		= 1,
		ES_Propagated	= 2,
	};

	struct Frame
	{
		Function*			function		= nullptr;
		const Instruction*	ip				= nullptr;
		const Instruction*	instructions	= nullptr;
		const Instruction*	instructionsEnd	= nullptr;
		Object*				thisObject		= nullptr;
		std::vector<Value>	variables;
		Array				anonymousParameters;
		ErrorState			errorState		= ES_NoError;
	};
	
	struct SymbolInfo
	{
		std::string name;
		int globalIndex;
	};

protected:
	int		ParseBytecode(const char* bytecode);
	void	RunCode();
	
	void	LoadElementFromArray(const Array* array, int index, Value* outValue) const;
	void	StoreElementInArray(Array* array, int index, const Value* newValue);
	void	LoadMemberFromObject(Object* object, unsigned hash, Value* outValue) const;
	void	StoreMemberInObject(Object* object, unsigned hash, const Value* newValue);
	void	Call(int argumentsCount);
	void	CallNative(int argumentsCount);
	void	DoBinaryOperation(int opCode);

	int		CurrentLineFromFrame(const Frame* frame) const;
	
	void	AddRootsToGrayList();
	
private:
	std::string							mErrorMessage;
	Logger&								mLogger;

	MemoryManager						mMemoryManager;
	
	std::deque<CodeObject>				mCodeObjects;
	std::vector<Value>					mConstants;
	std::deque<Function>				mConstantFunctions;
	std::deque<String>					mConstantStrings;
	std::vector<Value::NativeFunction>	mNativeFunctions;
	std::map<unsigned, SymbolInfo>		mSymbolInfos;
	
	std::map<int, Value>				mGlobalVariables;
	std::deque<Frame>					mStackFrames;
	Frame*								mCurrentFrame;
	Object*								mLastObject;
	std::vector<Value>					mStack;
	
	unsigned							mInstructionsProcessed;
};

}

#endif // _VIRTUAL_MACHINE_INCLUDED_
