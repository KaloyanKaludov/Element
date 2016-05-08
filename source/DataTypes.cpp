#include "DataTypes.h"

#include <algorithm>

namespace element
{

GarbageCollected::GarbageCollected(Value::Type type)
: next(nullptr)
, type(type)
, state(GC_White0)
{}


String::String()
: GarbageCollected(Value::VT_String)
{}

String::String(const std::string& str)
: GarbageCollected(Value::VT_String)
, str(str)
{}

String::String(const char* data, unsigned size)
: GarbageCollected(Value::VT_String)
, str(data, size)
{}


Array::Array()
: GarbageCollected(Value::VT_Array)
{}


Object::Member::Member()
: hash(0)
{}

Object::Member::Member(unsigned hash)
: hash(hash)
{}

Object::Member::Member(unsigned hash, const Value& value)
: hash(hash)
, value(value)
{}

bool Object::Member::operator<(const Member& o) const
{
	return hash < o.hash;
}

bool Object::Member::operator==(const Member& o) const
{
	return hash == o.hash;
}

Object::Object()
: GarbageCollected(Value::VT_Object)
, members(1) // at least a 'proto' member
{}


CodeObject::CodeObject(	Instruction* data, unsigned size, 
						std::pair<int, int>* lines, unsigned linesSize,
						int localVariablesCount, 
						int namedParametersCount )
: instructions(data, data + size)
, localVariablesCount(localVariablesCount)
, namedParametersCount(namedParametersCount)
, instructionLines(lines, lines + linesSize)
{
}

CodeObject::CodeObject(	Instruction* instructions, unsigned instructionsSize, 
						AskedVariable* closure, unsigned closureSize,
						std::pair<int, int>* lines, unsigned linesSize,
						int localVariablesCount, int namedParametersCount)
: closureMapping(closure, closure + closureSize)
, instructions(instructions, instructions + instructionsSize)
, localVariablesCount(localVariablesCount)
, namedParametersCount(namedParametersCount)
, instructionLines(lines, lines + linesSize)
{
}


Box::Box()
: GarbageCollected(Value::VT_Box)
{}


Function::Function(CodeObject* codeObject)
: GarbageCollected(Value::VT_Function)
, instructions(codeObject->instructions.data())
, instructionsEnd(codeObject->instructions.data() + codeObject->instructions.size())
, localVariablesCount(codeObject->localVariablesCount)
, namedParametersCount(codeObject->namedParametersCount)
, instructionLines(&codeObject->instructionLines)
, closureMapping(&codeObject->closureMapping)
{
}

Function::Function(const Function* o)
: GarbageCollected(Value::VT_Function)
, instructions(o->instructions)
, instructionsEnd(o->instructionsEnd)
, localVariablesCount(o->localVariablesCount)
, namedParametersCount(o->namedParametersCount)
, instructionLines(o->instructionLines)
, closureMapping(o->closureMapping)
{
}


Generator::Generator(GeneratorImplementation* implementation)
: GarbageCollected(Value::VT_NativeGenerator)
, implementation(implementation)
{}

Generator::~Generator()
{
	delete implementation; // virtual call
}


ArrayGenerator::ArrayGenerator(Array* array)
: array(array)
{
}

bool ArrayGenerator::has_value()
{
	return currentIndex < array->elements.size();
}

Value ArrayGenerator::next_value()
{
	return array->elements[currentIndex++];
}

void ArrayGenerator::UpdateGrayList(std::deque<GarbageCollected*>& grayList,
									GarbageCollected::State currentWhite)
{
	if( array->state == currentWhite )
		grayList.push_back(array);
}


StringGenerator::StringGenerator(String* str)
: str(str)
{
}

bool StringGenerator::has_value()
{
	return currentIndex < str->str.size();
}

Value StringGenerator::next_value()
{
	return int(str->str[currentIndex++]); // as ASCII character codes...
}

void StringGenerator::UpdateGrayList(std::deque<GarbageCollected*>& grayList,
									 GarbageCollected::State currentWhite)
{
	if( str->state == currentWhite )
		grayList.push_back(str);
}

}
