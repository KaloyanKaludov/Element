#include "Native.h"

#include <cmath>
#include <locale>

#include "VirtualMachine.h"
#include "DataTypes.h"
#include "Symbol.h"

namespace element
{

namespace nativefunctions
{

Value Type(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'type(value)' takes exactly one argument");
		return Value();
	}

	Value result = vm.GetMemoryManager().NewString();

	switch( args[0].type )
	{
	case Value::VT_Nil:
		result.string->str = "nil";
		break;
	case Value::VT_Int:
		result.string->str = "int";
		break;
	case Value::VT_Float:
		result.string->str = "float";
		break;
	case Value::VT_Bool:
		result.string->str = "bool";
		break;
	case Value::VT_String:
		result.string->str = "string";
		break;
	case Value::VT_Array:
		result.string->str = "array";
		break;
	case Value::VT_Object:
		result.string->str = "object";
		break;
	case Value::VT_Function:
		result.string->str = "function";
		break;
	case Value::VT_Iterator:
		result.string->str = "iterator";
		break;
	case Value::VT_NativeFunction:
		result.string->str = "native-function";
		break;
	case Value::VT_Error:
		result.string->str = "error";
		break;
	default:
		result.string->str = "<[???]>";
		break;
	}
	return result;
}

Value ThisCall(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() < 2 )
	{
		vm.SetError("function 'this_call(function, this, args...)' takes at least two arguments");
		return Value();
	}

	const Value& function = args[0];

	if( ! function.IsFunction() )
	{
		vm.SetError("function 'this_call(function, this, args...)' takes a function as a first argument");
		return Value();
	}

	const Value& object = args[1];

	if( ! object.IsObject() )
	{
		vm.SetError("function 'this_call(function, this, args...)' takes an object as a second argument");
		return Value();
	}

	std::vector<Value> thisCallArgs(args.begin() + 2, args.end());

	// TODO: This will create a new execution context. Do we really want that?
	Value result = vm.CallMemberFunction(object, function, thisCallArgs);

	return result;
}

Value GarbageCollect(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.empty() )
	{
		vm.GetMemoryManager().GarbageCollect();
	}
	else if( args[0].IsInt() )
	{
		int steps = args[0].AsInt();
		vm.GetMemoryManager().GarbageCollect(steps);
	}
	else
	{
		vm.SetError("function 'garbage_collect(steps)' takes a single integer as an argument");
	}

	return Value();
}

Value MemoryStats(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	MemoryManager& memoryManager = vm.GetMemoryManager();

	int strings		= memoryManager.GetHeapObjectsCount(Value::VT_String);
	int arrays		= memoryManager.GetHeapObjectsCount(Value::VT_Array);
	int objects		= memoryManager.GetHeapObjectsCount(Value::VT_Object);
	int functions	= memoryManager.GetHeapObjectsCount(Value::VT_Function);
	int boxes		= memoryManager.GetHeapObjectsCount(Value::VT_Box);
	int iterators	= memoryManager.GetHeapObjectsCount(Value::VT_Iterator);
	int errors		= memoryManager.GetHeapObjectsCount(Value::VT_Error);

	int total = strings + arrays + objects + functions + boxes + iterators + errors;

	Value data = memoryManager.NewObject();
	
	vm.SetMember(data, "heap_strings_count",	Value(strings));
	vm.SetMember(data, "heap_arrays_count",		Value(arrays));
	vm.SetMember(data, "heap_objects_count",	Value(objects));
	vm.SetMember(data, "heap_functions_count",	Value(functions));
	vm.SetMember(data, "heap_boxes_count",		Value(boxes));
	vm.SetMember(data, "heap_iterators_count",	Value(iterators));
	vm.SetMember(data, "heap_errors_count",		Value(errors));
	vm.SetMember(data, "heap_total_count",		Value(total));

	return data;
}

Value Print(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	for( const Value& arg : args )
	{
		std::string str = arg.AsString();
		std::string::size_type pos = str.find('\\');

		while( pos != std::string::npos && pos + 1 < str.size() )
		{
			switch( str[pos + 1] )
			{
			case 'n': str.replace(pos, 2, "\n"); break;
			case 'r': str.replace(pos, 2, "\r"); break;
			case 't': str.replace(pos, 2, "\t"); break;
			case '\\': str.replace(pos, 2, "\\"); break;
			}

			pos = str.find('\\');
		}

		printf("%s", str.c_str());
	}

	printf("\n");

	return int(args.size());
}

Value ToUpper(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'to_upper(string)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsString() )
	{
		vm.SetError("function 'to_upper(string)' takes a string as argument");
		return Value();
	}

	std::locale locale;
	std::string str = args[0].string->str;

	unsigned size = str.size();

	for( unsigned i = 0; i < size; ++i )
		str[i] = std::toupper(str[i], locale);

	return vm.GetMemoryManager().NewString(str);
}

Value ToLower(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'to_lower(string)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsString() )
	{
		vm.SetError("function 'to_lower(string)' takes a string as argument");
		return Value();
	}

	std::locale locale;
	std::string str = args[0].string->str;

	unsigned size = str.size();

	for( unsigned i = 0; i < size; ++i )
		str[i] = std::tolower(str[i], locale);

	return vm.GetMemoryManager().NewString(str);
}

Value Keys(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'keys(object)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsObject() )
	{
		vm.SetError("function 'keys(object)' takes an object as argument");
		return Value();
	}
	
	MemoryManager& memoryManager = vm.GetMemoryManager();
	
	Object* object = args[0].object;
	
	Array* keys = memoryManager.NewArray();
	std::string name;
	
	for( const Object::Member& member : object->members )
	{
		if( vm.GetNameFromHash(member.hash, &name) )
			keys->elements.push_back( memoryManager.NewString(name) );
		
		name.clear();
	}
	
	return keys;
}

Value MakeError(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'make_error(message)' takes exactly one argument");
		return Value();
	}

	Value::Type type = args[0].type;

	if( type != Value::VT_String )
	{
		vm.SetError("function 'make_error(message)' takes only a string as an argument");
		return Value();
	}

	const std::string& str = args[0].string->str;

	return vm.GetMemoryManager().NewError(str);
}

Value IsError(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'is_error(value)' takes exactly one argument");
		return Value();
	}

	return args[0].IsError();
}

Value MakeCoroutine(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'make_coroutine(function)' takes exactly one argument");
		return Value();
	}

	Value::Type type = args[0].type;

	if( type != Value::VT_Function )
	{
		vm.SetError("function 'make_coroutine(function)' takes only a function as an argument");
		return Value();
	}

	return vm.GetMemoryManager().NewCoroutine( args[0].function );
}

Value MakeIterator(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'make_iterator(value)' takes exactly one argument");
		return Value();
	}
	
	Iterator* iterator = vm.MakeIteratorForValue( args[0] );
	
	if( iterator )
		return iterator;
	
	if( args[0].type == Value::VT_Function && 
		args[0].function->executionContext == nullptr )
	{
		vm.SetError("function 'make_iterator(value)': Cannot iterate a function. Only coroutine instances are iterable.");
	}
	else
	{
		vm.SetError("function 'make_iterator(value)': Value is not iterable.");
	}

	return Value();
}

Value IteratorHasNext(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'iterator_has_next(iterator)' takes exactly one argument");
		return Value();
	}
	
	if( args[0].type != Value::VT_Iterator )
	{
		vm.SetError("function 'iterator_get_next(iterator)' takes an iterator as a first argument");
		return Value();
	}
	
	IteratorImplementation* ii = args[0].iterator->implementation;
	
	Value result = vm.CallMemberFunction(ii->thisObjectUsed, ii->hasNextFunction, {});
	
	if( vm.HasError() )
		return Value();
	
	return result;
}

Value IteratorGetNext(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'iterator_get_next(iterator)' takes exactly one argument");
		return Value();
	}
	
	if( args[0].type != Value::VT_Iterator )
	{
		vm.SetError("function 'iterator_get_next(iterator)' takes an iterator as a first argument");
		return Value();
	}
	
	IteratorImplementation* ii = args[0].iterator->implementation;
	
	Value result = vm.CallMemberFunction(ii->thisObjectUsed, ii->getNextFunction, {});
	
	if( vm.HasError() )
		return Value();
	
	return result;
}


struct RangeIterator : public IteratorImplementation
{
	int	from = 0;
	int	to = 0;
	int step = 1;
	
	RangeIterator()
	{
		hasNextFunction = Value([](VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) -> Value
		{
			RangeIterator* self = static_cast<RangeIterator*>(thisObject.iterator->implementation);
			
			return self->from < self->to;
		});
		
		getNextFunction = Value([](VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) -> Value
		{
			RangeIterator* self = static_cast<RangeIterator*>(thisObject.iterator->implementation);
			
			int result = self->from;
			self->from += self->step;
			return result;
		});
	}
};


Value Range(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args) // TODO check for reversed ranges like 'range(10, 0)'
{
	if( args.size() == 1 )
	{
		if( ! args[0].IsInt() )
		{
			vm.SetError("function 'range(max)' takes an integer as argument");
			return Value();
		}

		RangeIterator* rangeIterator = new RangeIterator();

		rangeIterator->to = args[0].AsInt();

		return vm.GetMemoryManager().NewIterator(rangeIterator);
	}
	else if( args.size() == 2 )
	{
		if( ! args[0].IsInt() || ! args[1].IsInt() )
		{
			vm.SetError("function 'range(min,max)' takes integers as arguments");
			return Value();
		}

		RangeIterator* rangeIterator = new RangeIterator();

		rangeIterator->from = args[0].AsInt();
		rangeIterator->to = args[1].AsInt();

		return vm.GetMemoryManager().NewIterator(rangeIterator);
	}
	else if( args.size() == 3 )
	{
		if( ! args[0].IsInt() || ! args[1].IsInt() || ! args[2].IsInt() )
		{
			vm.SetError("function 'range(min,max,step)' takes integers as arguments");
			return Value();
		}

		RangeIterator* rangeIterator = new RangeIterator();

		rangeIterator->from = args[0].AsInt();
		rangeIterator->to = args[1].AsInt();
		rangeIterator->step = args[2].AsInt();

		return vm.GetMemoryManager().NewIterator(rangeIterator);
	}

	vm.SetError("'range' can only be 'range(max)', 'range(min,max)' or 'range(min,max,step)'");
	return Value();
}

Value Each(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'each(iterable, function)' takes exactly two arguments");
		return Value();
	}

	const Value& function = args[1];

	if( ! function.IsFunction() )
	{
		vm.SetError("function 'each(iterable, function)': second argument is not a function");
		return Value();
	}
	
	if( Iterator* iterator = vm.MakeIteratorForValue( args[0] ) )
	{
		Value result;
		std::vector<Value> noArgs;
		Value& objectUsed	= iterator->implementation->thisObjectUsed;
		Value& hasNext		= iterator->implementation->hasNextFunction;
		Value& getNext		= iterator->implementation->getNextFunction;
		
		while( true )
		{
			result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
			
			if( vm.HasError() )
				return Value();
			
			if( !result.AsBool() )
				break;
	
			result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
			
			if( vm.HasError() )
				return Value();
				
			vm.CallFunction(function, {result});

			if( vm.HasError() )
				return Value();
		}
	}
	else
	{
		vm.SetError("function 'each(iterable, function)': first argument not interable");
	}
	
	return Value();
}

Value Times(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'times(n, function)' takes exactly two arguments");
		return Value();
	}

	if( ! args[0].IsInt() )
	{
		vm.SetError("function 'times(n, function)': first argument is not an integer");
		return Value();
	}

	int times = args[0].AsInt();
	const Value& function = args[1];

	if( ! function.IsFunction() )
	{
		vm.SetError("function 'times(n, function)': second argument is not a function");
		return Value();
	}

	for( int i = 0; i < times; ++i )
	{
		vm.CallFunction(function, {i});

		if( vm.HasError() )
			return Value();
	}

	return Value();
}

Value Count(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'count(iterable, function)' takes exactly two arguments");
		return Value();
	}

	const Value& function = args[1];

	if( ! function.IsFunction() )
	{
		vm.SetError("function 'count(iterable, function)': second argument is not a function");
		return Value();
	}

	if( Iterator* iterator = vm.MakeIteratorForValue( args[0] ) )
	{
		Value result;
		std::vector<Value> noArgs;
		Value& objectUsed	= iterator->implementation->thisObjectUsed;
		Value& hasNext		= iterator->implementation->hasNextFunction;
		Value& getNext		= iterator->implementation->getNextFunction;
		
		int counter = 0;
		
		while( true )
		{
			result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
			
			if( vm.HasError() )
				return Value();
			
			if( !result.AsBool() )
				break;
	
			result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
			
			if( vm.HasError() )
				return Value();
				
			result = vm.CallFunction(function, {result});

			if( vm.HasError() )
				return Value();

			if( result.AsBool() )
				++counter;
		}
		
		return counter;
	}
	
	vm.SetError("function 'count(iterable, function)': first argument not interable");
	return Value();
}

Value Map(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'map(iterable, function)' takes exactly two arguments");
		return Value();
	}

	const Value& function = args[1];

	if( ! function.IsFunction() )
	{
		vm.SetError("function 'map(iterable, function)': second argument is not a function");
		return Value();
	}
	
	if( Iterator* iterator = vm.MakeIteratorForValue( args[0] ) )
	{
		Value result;
		std::vector<Value> noArgs;
		Value& objectUsed	= iterator->implementation->thisObjectUsed;
		Value& hasNext		= iterator->implementation->hasNextFunction;
		Value& getNext		= iterator->implementation->getNextFunction;
		
		Value mapped = vm.GetMemoryManager().NewArray();
		
		while( true )
		{
			result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
			
			if( vm.HasError() )
				return Value();
			
			if( !result.AsBool() )
				break;
	
			result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
			
			if( vm.HasError() )
				return Value();
				
			result = vm.CallFunction(function, {result});

			if( vm.HasError() )
				return Value();
			
			mapped.array->elements.push_back(result);
		}
		
		return mapped;
	}
	
	vm.SetError("function 'map(iterable, function)': first argument not interable");
	return Value();
}

Value Filter(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'filter(iterable, predicate)' takes exactly two arguments");
		return Value();
	}

	const Value& function = args[1];

	if( ! function.IsFunction() )
	{
		vm.SetError("function 'filter(iterable, predicate)': second argument is not a function");
		return Value();
	}
	
	if( Iterator* iterator = vm.MakeIteratorForValue( args[0] ) )
	{
		Value result;
		Value item;
		std::vector<Value> noArgs;
		Value& objectUsed	= iterator->implementation->thisObjectUsed;
		Value& hasNext		= iterator->implementation->hasNextFunction;
		Value& getNext		= iterator->implementation->getNextFunction;
		
		Value filtered = vm.GetMemoryManager().NewArray();
		
		while( true )
		{
			result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
			
			if( vm.HasError() )
				return Value();
			
			if( !result.AsBool() )
				break;
	
			item = vm.CallMemberFunction(objectUsed, getNext, noArgs);
			
			if( vm.HasError() )
				return Value();
				
			result = vm.CallFunction(function, {item});

			if( vm.HasError() )
				return Value();
			
			if( result.AsBool() )
				filtered.array->elements.push_back(item);
		}
		
		return filtered;
	}
	
	vm.SetError("function 'filter(iterable, function)': first argument not interable");
	return Value();
}

Value Reduce(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'reduce(iterable, function)' takes exactly two arguments");
		return Value();
	}

	const Value& function = args[1];

	if( ! function.IsFunction() )
	{
		vm.SetError("function 'reduce(iterable, function)': second argument is not a function");
		return Value();
	}
	
	if( Iterator* iterator = vm.MakeIteratorForValue( args[0] ) )
	{
		Value result;
		std::vector<Value> noArgs;
		Value& objectUsed	= iterator->implementation->thisObjectUsed;
		Value& hasNext		= iterator->implementation->hasNextFunction;
		Value& getNext		= iterator->implementation->getNextFunction;
		
		Value reduced;
		result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
		
		if( vm.HasError() )
			return Value();
		
		if( result.AsBool() )
		{
			reduced = vm.CallMemberFunction(objectUsed, getNext, noArgs);
			
			while( true )
			{
				result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
				
				if( vm.HasError() )
					return Value();
				
				if( !result.AsBool() )
					break;
		
				result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
				
				if( vm.HasError() )
					return Value();
				
				reduced = vm.CallFunction(function, {reduced, result});

				if( vm.HasError() )
					return Value();
			}
		}
		
		return reduced;
	}
	
	vm.SetError("function 'reduce(iterable, function)': first argument not interable");
	return Value();
}

Value All(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	unsigned argsSize = args.size();

	if( argsSize < 1 || argsSize > 2 )
	{
		vm.SetError("function 'all(iterable, [predicate])' takes one or two arguments");
		return Value();
	}

	if( Iterator* iterator = vm.MakeIteratorForValue( args[0] ) )
	{
		Value result;
		std::vector<Value> noArgs;
		Value& objectUsed	= iterator->implementation->thisObjectUsed;
		Value& hasNext		= iterator->implementation->hasNextFunction;
		Value& getNext		= iterator->implementation->getNextFunction;
		
		if( argsSize == 1 )
		{
			while( true )
			{
				result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
				
				if( vm.HasError() )
					return Value();
				
				if( !result.AsBool() )
					break;
		
				result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
				
				if( vm.HasError() )
					return Value();
					
				if( !result.AsBool() )
					return false;
			}
			
			return true;
		}
		else
		{
			const Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'all(iterable, [predicate])': second argument is not a function");
				return Value();
			}
			
			while( true )
			{
				result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
				
				if( vm.HasError() )
					return Value();
				
				if( !result.AsBool() )
					break;
		
				result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
				
				if( vm.HasError() )
					return Value();
					
				result = vm.CallFunction(function, {result});

				if( vm.HasError() )
					return Value();

				if( !result.AsBool() )
					return false;
			}
			
			return true;
		}
	}
	
	vm.SetError("function 'all(iterable, [predicate])': first argument not interable");
	return Value();
}

Value Any(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	unsigned argsSize = args.size();

	if( argsSize < 1 || argsSize > 2 )
	{
		vm.SetError("function 'any(iterable, [predicate])' takes one or two arguments");
		return Value();
	}
	
	if( Iterator* iterator = vm.MakeIteratorForValue( args[0] ) )
	{
		Value result;
		std::vector<Value> noArgs;
		Value& objectUsed	= iterator->implementation->thisObjectUsed;
		Value& hasNext		= iterator->implementation->hasNextFunction;
		Value& getNext		= iterator->implementation->getNextFunction;
		
		if( argsSize == 1 )
		{
			while( true )
			{
				result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
				
				if( vm.HasError() )
					return Value();
				
				if( !result.AsBool() )
					break;
		
				result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
				
				if( vm.HasError() )
					return Value();
					
				if( result.AsBool() )
					return true;
			}
			
			return false;
		}
		else // argsSize == 2
		{
			const Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'any(iterable, [predicate])': second argument is not a function");
				return Value();
			}
			
			while( true )
			{
				result = vm.CallMemberFunction(objectUsed, hasNext, noArgs);
				
				if( vm.HasError() )
					return Value();
				
				if( !result.AsBool() )
					break;
		
				result = vm.CallMemberFunction(objectUsed, getNext, noArgs);
				
				if( vm.HasError() )
					return Value();
					
				result = vm.CallFunction(function, {result});

				if( vm.HasError() )
					return Value();

				if( result.AsBool() )
					return true;
			}
			
			return false;
		}
	}
	
	vm.SetError("function 'any(iterable, [predicate])': first argument not interable");
	return Value();
}

Value Min(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	vm.SetError("function 'min' is not implemented");
	return Value();
}

Value Max(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	vm.SetError("function 'max' is not implemented");
	return Value();
}

Value Sort(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	vm.SetError("function 'sort' is not implemented");
	return Value();
}

Value Abs(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'abs(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'abs(number)': number must be int or float");
	}

	if( args[0].IsInt() )
		return std::abs(args[0].AsInt());
	return std::abs(args[0].AsFloat());
}

Value Floor(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'floor(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'floor(number)': number must be int or float");
	}

	return std::floor(args[0].AsFloat());
}

Value Ceil(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'ceil(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'ceil(number)': number must be int or float");
	}

	return std::ceil(args[0].AsFloat());
}

Value Round(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'round(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'round(number)': number must be int or float");
	}

	return std::round(args[0].AsFloat());
}

Value Sqrt(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'sqrt(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'sqrt(number)': number must be int or float");
	}

	return std::sqrt(args[0].AsFloat());
}

Value Sin(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'sin(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'sin(number)': number must be int or float");
	}

	return std::sin(args[0].AsFloat());
}

Value Cos(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'cos(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'cos(number)': number must be int or float");
	}

	return std::cos(args[0].AsFloat());
}

Value Tan(VirtualMachine& vm, const Value& thisObject, const std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'tan(number)' takes exactly one argument");
		return Value();
	}

	if( ! args[0].IsNumber() )
	{
		vm.SetError("function 'tan(number)': number must be int or float");
	}

	return std::tan(args[0].AsFloat());
}

}

}
