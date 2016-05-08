#ifndef _NATIVE_INCLUDED_
#define _NATIVE_INCLUDED_

#include "Value.h"

namespace element
{

namespace nativefunctions
{

Value Type			(VirtualMachine& vm, std::vector<Value>& args);
Value ThisCall		(VirtualMachine& vm, std::vector<Value>& args);
Value GarbageCollect(VirtualMachine& vm, std::vector<Value>& args);
Value MemoryStats	(VirtualMachine& vm, std::vector<Value>& args);
Value Print			(VirtualMachine& vm, std::vector<Value>& args);
Value ToUpper		(VirtualMachine& vm, std::vector<Value>& args);
Value ToLower		(VirtualMachine& vm, std::vector<Value>& args);
Value MakeGenerator	(VirtualMachine& vm, std::vector<Value>& args);
Value Range			(VirtualMachine& vm, std::vector<Value>& args);
Value Each			(VirtualMachine& vm, std::vector<Value>& args);
Value Times			(VirtualMachine& vm, std::vector<Value>& args);
Value Count			(VirtualMachine& vm, std::vector<Value>& args);
Value Map			(VirtualMachine& vm, std::vector<Value>& args);
Value Filter		(VirtualMachine& vm, std::vector<Value>& args);
Value Reduce		(VirtualMachine& vm, std::vector<Value>& args);
Value All			(VirtualMachine& vm, std::vector<Value>& args);
Value Any			(VirtualMachine& vm, std::vector<Value>& args);
Value Min			(VirtualMachine& vm, std::vector<Value>& args);
Value Max			(VirtualMachine& vm, std::vector<Value>& args);
Value Sort			(VirtualMachine& vm, std::vector<Value>& args);
Value Abs			(VirtualMachine& vm, std::vector<Value>& args);
Value Floor			(VirtualMachine& vm, std::vector<Value>& args);
Value Ceil			(VirtualMachine& vm, std::vector<Value>& args);
Value Round			(VirtualMachine& vm, std::vector<Value>& args);
Value Sqrt			(VirtualMachine& vm, std::vector<Value>& args);
Value Sin			(VirtualMachine& vm, std::vector<Value>& args);
Value Cos			(VirtualMachine& vm, std::vector<Value>& args);
Value Tan			(VirtualMachine& vm, std::vector<Value>& args);

}

}

#endif // _NATIVE_INCLUDED_
