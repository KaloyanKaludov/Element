#ifndef _NATIVE_INCLUDED_
#define _NATIVE_INCLUDED_

#include "Value.h"

namespace element
{

namespace nativefunctions
{

Value Type				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value ThisCall			(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value GarbageCollect	(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value MemoryStats		(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Print				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value ToUpper			(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value ToLower			(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Keys				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value MakeError			(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value IsError			(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value MakeCoroutine		(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value MakeIterator		(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value IteratorHasNext	(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value IteratorGetNext	(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Range				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Each				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Times				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Count				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Map				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Filter			(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Reduce			(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value All				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Any				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Min				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Max				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Sort				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Abs				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Floor				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Ceil				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Round				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Sqrt				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Sin				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Cos				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);
Value Tan				(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args);

}

}

#endif // _NATIVE_INCLUDED_
