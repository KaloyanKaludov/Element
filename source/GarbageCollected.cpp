#include "GarbageCollected.h"

#include "DataTypes.h"
#include "VirtualMachine.h"

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


Error::Error()
: GarbageCollected(Value::VT_Error)
{}

Error::Error(const std::string& str)
: GarbageCollected(Value::VT_Error)
, errorString(str)
{}

Error::Error(const char* data, unsigned size)
: GarbageCollected(Value::VT_Error)
, errorString(data, size)
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


Box::Box()
: GarbageCollected(Value::VT_Box)
{}


Function::Function(const CodeObject* codeObject)
: GarbageCollected(Value::VT_Function)
, codeObject(codeObject)
, executionContext(nullptr)
{
}

Function::Function(const Function* o)
: GarbageCollected(Value::VT_Function)
, codeObject(o->codeObject)
, freeVariables(o->freeVariables)
, executionContext(nullptr)
{
}


Iterator::Iterator(IteratorImplementation* implementation)
: GarbageCollected(Value::VT_Iterator)
, implementation(implementation)
{
	if( implementation->thisObjectUsed.IsNil() )
		implementation->thisObjectUsed = Value(this);
}

Iterator::~Iterator()
{
	delete implementation; // virtual call
}


ArrayIterator::ArrayIterator(Array* array)
: array(array)
{
	hasNextFunction = Value([](VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) -> Value
	{
		ArrayIterator* self = static_cast<ArrayIterator*>(thisObject.iterator->implementation);
		
		return self->currentIndex < self->array->elements.size();
	});
	
	getNextFunction = Value([](VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) -> Value
	{
		ArrayIterator* self = static_cast<ArrayIterator*>(thisObject.iterator->implementation);
		
		return self->array->elements[self->currentIndex++];
	});
}

void ArrayIterator::UpdateGrayList(std::deque<GarbageCollected*>& grayList,
									GarbageCollected::State currentWhite)
{
	if( array->state == currentWhite )
		grayList.push_back(array);
}


StringIterator::StringIterator(String* str)
: str(str)
{
	hasNextFunction = Value([](VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) -> Value
	{
		StringIterator* self = static_cast<StringIterator*>(thisObject.iterator->implementation);
		
		return self->currentIndex < self->str->str.size();
	});
	
	getNextFunction = Value([](VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) -> Value
	{
		StringIterator* self = static_cast<StringIterator*>(thisObject.iterator->implementation);
		
		char c = self->str->str[self->currentIndex++];
		
		return vm.GetMemoryManager().NewString(&c, 1);
	});
}

void StringIterator::UpdateGrayList(std::deque<GarbageCollected*>& grayList,
									 GarbageCollected::State currentWhite)
{
	if( str->state == currentWhite )
		grayList.push_back(str);
}


ObjectIterator::ObjectIterator(const Value& object, const Value& hasNext, const Value& getNext)
{
	thisObjectUsed	= object;
	hasNextFunction	= hasNext;
	getNextFunction	= getNext;
}

void ObjectIterator::UpdateGrayList(std::deque<GarbageCollected*>& grayList,
									GarbageCollected::State currentWhite)
{
	if( thisObjectUsed.object->state == currentWhite )
		grayList.push_back(thisObjectUsed.object);
}


CoroutineIterator::CoroutineIterator(Function* coroutine)
{
	hasNextFunction = Value([](VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) -> Value
	{
		CoroutineIterator* self = static_cast<CoroutineIterator*>(thisObject.iterator->implementation);
		
		return self->getNextFunction.function->executionContext->state != ExecutionContext::CRS_Finished;
	});
	
	getNextFunction = coroutine;
}

void CoroutineIterator::UpdateGrayList(std::deque<GarbageCollected*>& grayList,
									GarbageCollected::State currentWhite)
{
	if( getNextFunction.function->state == currentWhite )
		grayList.push_back(getNextFunction.function);
}

}
