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

Value Type(VirtualMachine& vm, std::vector<Value>& args)
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
	case Value::VT_NativeGenerator:
		result.string->str = "native-generator";
		break;
	case Value::VT_NativeFunction:
		result.string->str = "native-function";
		break;
	default:
		result.string->str = "<[???]>";
		break;
	}
	return result;
}

Value ThisCall(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.size() < 2 )
	{
		vm.SetError("function 'this_call(function, this, args...)' takes at least two arguments");
		return Value();
	}
	
	Value& function = args[0];
	
	if( ! function.IsFunction() )
	{
		vm.SetError("function 'this_call(function, this, args...)' takes a function as a first argument");
		return Value();
	}
	
	Value& thisObject = args[1];
	
	if( ! thisObject.IsObject() )
	{
		vm.SetError("function 'this_call(function, this, args...)' takes an object as a second argument");
		return Value();
	}
	
	std::vector<Value> thisCallArgs(args.begin() + 2, args.end());
		
	Value result = vm.CallMemberFunction(thisObject, function, thisCallArgs);
		
	return result;
}

Value GarbageCollect(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.empty() )
	{
		vm.GarbageCollect();
	}
	else if( args[0].IsInt() )
	{
		int steps = args[0].AsInt();
		vm.GarbageCollect(steps);
	}
	else
	{
		vm.SetError("function 'garbage_collect(steps)' takes a single integer as argument");
	}

	return Value();
}

Value MemoryStats(VirtualMachine& vm, std::vector<Value>& args)
{
	Value data = vm.GetMemoryManager().NewObject();

	int strings		= int(vm.GetMemoryManager().GetHeapObjectsCount(Value::VT_String));
	int arrays		= int(vm.GetMemoryManager().GetHeapObjectsCount(Value::VT_Array));
	int objects		= int(vm.GetMemoryManager().GetHeapObjectsCount(Value::VT_Object));
	int functions	= int(vm.GetMemoryManager().GetHeapObjectsCount(Value::VT_Function));
	int boxes		= int(vm.GetMemoryManager().GetHeapObjectsCount(Value::VT_Box));
	int generators	= int(vm.GetMemoryManager().GetHeapObjectsCount(Value::VT_NativeGenerator));

	int total = strings + arrays + objects + functions + boxes + generators;

	vm.SetMember(data, vm.GetHashFromName("heap_strings_count"),	Value(strings));
	vm.SetMember(data, vm.GetHashFromName("heap_arrays_count"),		Value(arrays));
	vm.SetMember(data, vm.GetHashFromName("heap_objects_count"),	Value(objects));
	vm.SetMember(data, vm.GetHashFromName("heap_functions_count"),	Value(functions));
	vm.SetMember(data, vm.GetHashFromName("heap_boxes_count"),		Value(boxes));
	vm.SetMember(data, vm.GetHashFromName("heap_generators_count"),	Value(generators));
	vm.SetMember(data, vm.GetHashFromName("heap_total_count"),		Value(total));

	return data;
}

Value Print(VirtualMachine& vm, std::vector<Value>& args)
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

Value ToUpper(VirtualMachine& vm, std::vector<Value>& args)
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

Value ToLower(VirtualMachine& vm, std::vector<Value>& args)
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

Value MakeGenerator(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.size() != 1 )
	{
		vm.SetError("function 'make_generator(value)' takes exactly one argument");
		return Value();
	}
	
	Value::Type type = args[0].type;
	
	if( type == Value::VT_Array )
		return vm.GetMemoryManager().NewGeneratorArray( args[0].array );
	else if( type == Value::VT_String )
		return vm.GetMemoryManager().NewGeneratorString( args[0].string );
	
	vm.SetError("function 'make_generator(value)' takes only arrays and strings as arguments");
	return Value();
}

struct RangeGenerator : public GeneratorImplementation
{
	int	from = 0;
	int	to = 0;
	int step = 1;

	virtual bool has_value() override;
	virtual Value next_value() override;
	
	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) override;
};

bool RangeGenerator::has_value()
{
	return from < to;
}

Value RangeGenerator::next_value()
{
	int result = from;
	from += step;
	return result;
}

void RangeGenerator::UpdateGrayList(std::deque<GarbageCollected*>& grayList,
									GarbageCollected::State currentWhite)
{
}

Value Range(VirtualMachine& vm, std::vector<Value>& args) // TODO check for reversed ranges like 'range(10, 0)'
{
	if( args.size() == 1 )
	{
		if( ! args[0].IsInt() )
		{
			vm.SetError("function 'range(max)' takes an integer as argument");
			return Value();
		}
		
		RangeGenerator* rangeGenerator = new RangeGenerator();
		
		rangeGenerator->to = args[0].AsInt();
		
		return vm.GetMemoryManager().NewGeneratorNative(rangeGenerator);
	}
	else if( args.size() == 2 )
	{
		if( ! args[0].IsInt() || ! args[1].IsInt() )
		{
			vm.SetError("function 'range(min,max)' takes integers as arguments");
			return Value();
		}
		
		RangeGenerator* rangeGenerator = new RangeGenerator();
		
		rangeGenerator->from = args[0].AsInt();
		rangeGenerator->to = args[1].AsInt();
		
		return vm.GetMemoryManager().NewGeneratorNative(rangeGenerator);
	}
	else if( args.size() == 3 )
	{
		if( ! args[0].IsInt() || ! args[1].IsInt() || ! args[2].IsInt() )
		{
			vm.SetError("function 'range(min,max,step)' takes integers as arguments");
			return Value();
		}
		
		RangeGenerator* rangeGenerator = new RangeGenerator();
		
		rangeGenerator->from = args[0].AsInt();
		rangeGenerator->to = args[1].AsInt();
		rangeGenerator->step = args[2].AsInt();
		
		return vm.GetMemoryManager().NewGeneratorNative(rangeGenerator);
	}
	
	vm.SetError("'range' can only be 'range(max)', 'range(min,max)' or 'range(min,max,step)'");
	return Value();
}

Value Each(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'each(iterable, function)' takes exactly two arguments");
		return Value();
	}
	
	Value& function = args[1];
	
	if( ! function.IsFunction() )
	{
		vm.SetError("function 'each(iterable, function)': second argument is not a function");
		return Value();
	}
	
	if( args[0].IsArray() )
	{
		auto& array = args[0].array->elements;
		
		int size = int(array.size());
		for( int i = 0; i < size; ++i )
		{
			vm.CallFunction(function, {array[i], i});
			
			if( vm.HasError() )
				return Value();
		}
	}
	else if( args[0].IsNativeGenerator() )
	{
		Generator* generator = args[0].nativeGenerator;
		
		while( generator->implementation->has_value() )
		{
			vm.CallFunction(function, {generator->implementation->next_value()});
			
			if( vm.HasError() )
				return Value();
		}
	}
	else if( args[0].IsObject() )
	{
		Value& object = args[0];
				
		Value has_value = vm.GetMember(object, Symbol::HasValueHash);
		
		if( ! has_value.IsFunction() )
		{
			vm.SetError("function 'each(iterable, function)': iterable doesn't have 'has_value' function");
			return Value();
		}
		
		Value next_value = vm.GetMember(object, Symbol::NextValueHash);
		
		if( ! next_value.IsFunction() )
		{
			vm.SetError("function 'each(iterable, function)': iterable doesn't have 'next_value' function");
			return Value();
		}
		
		while( vm.CallMemberFunction(object, has_value, {}).AsBool() )
		{
			if( vm.HasError() )
				return Value();
			
			Value nextValue = vm.CallMemberFunction(object, next_value, {});
			
			if( vm.HasError() )
				return Value();
			
			vm.CallFunction(function, {nextValue});
			
			if( vm.HasError() )
				return Value();
		}
	}
	else
	{
		vm.SetError("function 'each(iterable, function)': iterable must be array, object or native-generator");
	}
	
	return Value();
}

Value Times(VirtualMachine& vm, std::vector<Value>& args)
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
	Value& function = args[1];
	
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

Value Count(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'count(array, function)' takes exactly two arguments");
		return Value();
	}
	
	if( ! args[0].IsArray() )
	{
		vm.SetError("function 'count(array, function)': first argument is not an array");
		return Value();
	}
	
	Value& array = args[0];
	Value& function = args[1];
	
	if( ! function.IsFunction() )
	{
		vm.SetError("function 'count(array, function)': second argument is not a function");
		return Value();
	}
	
	int counter = 0;

	for( const Value& element : array.array->elements )
	{
		Value result = vm.CallFunction(function, {element});
		
		if( vm.HasError() )
			return Value();
		
		if( result.AsBool() )
			++counter;
	}
	
	return counter;
}

Value Map(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'map(array, function)' takes exactly two arguments");
		return Value();
	}
	
	if( ! args[0].IsArray() )
	{
		vm.SetError("function 'map(array, function)': first argument is not an array");
		return Value();
	}
	
	auto& array = args[0].array->elements;
	Value& function = args[1];
	
	if( ! function.IsFunction() )
	{
		vm.SetError("function 'map(array, function)': second argument is not a function");
		return Value();
	}
	
	Value result = vm.GetMemoryManager().NewArray();
	
	result.array->elements.reserve(array.size());
	
	int size = int(array.size());
	for( int i = 0; i < size; ++i )
	{
		result.array->elements.push_back( vm.CallFunction(function, {array[i], i}) );
		
		if( vm.HasError() )
			return Value();
	}
	
	return result;
}

Value Filter(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'filter(array, predicate)' takes exactly two arguments");
		return Value();
	}
	
	if( ! args[0].IsArray() )
	{
		vm.SetError("function 'filter(array, predicate)': first argument is not an array");
		return Value();
	}
	
	auto& array = args[0].array->elements;
	Value& function = args[1];
	
	if( ! function.IsFunction() )
	{
		vm.SetError("function 'filter(array, predicate)': second argument is not a function");
		return Value();
	}

	Value result = vm.GetMemoryManager().NewArray();

	int size = int(array.size());
	for( int i = 0; i < size; ++i )
	{
		bool include = vm.CallFunction(function, {array[i], i}).AsBool();
		
		if( vm.HasError() )
			return Value();
		
		if( include )
			result.array->elements.push_back(array[i]);
	}
	
	return result;
}

Value Reduce(VirtualMachine& vm, std::vector<Value>& args)
{
	if( args.size() != 2 )
	{
		vm.SetError("function 'reduce(iterable, function)' takes exactly two arguments");
		return Value();
	}
	
	Value& function = args[1];
	
	if( ! function.IsFunction() )
	{
		vm.SetError("function 'reduce(iterable, function)': second argument is not a function");
		return Value();
	}
	
	Value result;

	if( args[0].IsArray() )
	{
		auto& array = args[0].array->elements;

		int size = int(array.size());

		if( size > 0 )
		{
			result = array[0];
			for( int i = 1; i < size; ++i )
			{
				result = vm.CallFunction(function, {result, array[i]});
				
				if( vm.HasError() )
					return Value();
			}
		}
	}
	else if( args[0].IsNativeGenerator() )
	{
		Generator* generator = args[0].nativeGenerator;

		if( generator->implementation->has_value() )
			result = generator->implementation->next_value();

		while( generator->implementation->has_value() )
		{
			result = vm.CallFunction(function, {result, generator->implementation->next_value()});
			
			if( vm.HasError() )
				return Value();
		}
	}
	else if( args[0].IsObject() )
	{
		Value& object = args[0];
		
		Value has_value = vm.GetMember(object, Symbol::HasValueHash);
		
		if( ! has_value.IsFunction() )
		{
			vm.SetError("function 'reduce(iterable, function)': iterable doesn't have 'has_value' function");
			return Value();
		}
		
		Value next_value = vm.GetMember(object, Symbol::NextValueHash);
		
		if( ! next_value.IsFunction() )
		{
			vm.SetError("function 'reduce(iterable, function)': iterable doesn't have 'next_value' function");
			return Value();
		}
		
		if( vm.CallMemberFunction(object, has_value, {}).AsBool() )
		{
			if( vm.HasError() )
				return Value();
			
			result = vm.CallMemberFunction(object, next_value, {});
			
			if( vm.HasError() )
				return Value();
		}
		
		while( vm.CallMemberFunction(object, has_value, {}).AsBool() )
		{
			if( vm.HasError() )
				return Value();
			
			Value nextValue = vm.CallMemberFunction(object, next_value, {});
			
			if( vm.HasError() )
				return Value();
			
			result = vm.CallFunction(function, {result, nextValue});
			
			if( vm.HasError() )
				return Value();
		}
	}
	else
	{
		vm.SetError("function 'reduce(iterable, function)': iterable must be array, object or native-generator");
	}
	
	return result;
}

Value All(VirtualMachine& vm, std::vector<Value>& args)
{
	unsigned argsSize = args.size();

	if( argsSize < 1 )
	{
		vm.SetError("function 'all' takes one or two arguments");
		return Value();
	}

	if( args[0].IsArray() )
	{
		auto& array = args[0].array->elements;

		if( argsSize == 1 )
		{
			for( const Value& element : array )
				if( !element.AsBool() )
					return false;
			
			return true;
		}
		else if( argsSize == 2 )
		{
			Value result;
			Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'all(iterable, function)': second argument is not a function");
				return Value();
			}

			for( const Value& element : array )
			{
				result = vm.CallFunction(function, {element});

				if( vm.HasError() )
				return Value();

				if( !result.AsBool() )
					return false;
			}

			return true;
		}
		else
		{
			vm.SetError("function 'all' takes one or two arguments");
			return Value();
		}
	}
	else if( args[0].IsNativeGenerator() )
	{
		Generator* generator = args[0].nativeGenerator;

		if( argsSize == 1 )
		{
			while( generator->implementation->has_value() )
				if( !generator->implementation->next_value().AsBool() )
					return false;

			return true;
		}
		else if( argsSize == 2 )
		{
			Value result;
			Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'all(iterable, function)': second argument is not a function");
				return Value();
			}

			while( generator->implementation->has_value() )
			{
				result = vm.CallFunction(function, {generator->implementation->next_value()});

				if( vm.HasError() )
					return Value();

				if( !result.AsBool() )
					return false;
			}

			return true;
		}
		else
		{
			vm.SetError("function 'all' takes one or two arguments");
			return Value();
		}
	}
	else if( args[0].IsObject() )
	{
		Value& object = args[0];

		Value has_value = vm.GetMember(object, Symbol::HasValueHash);

		if( !has_value.IsFunction() )
		{
			vm.SetError("function 'all(iterable)': iterable doesn't have 'has_value' function");
			return Value();
		}

		Value next_value = vm.GetMember(object, Symbol::NextValueHash);

		if( !next_value.IsFunction() )
		{
			vm.SetError("function 'all(iterable)': iterable doesn't have 'next_value' function");
			return Value();
		}

		if( argsSize == 1 )
		{
			while( vm.CallMemberFunction(object, has_value, {}).AsBool() )
			{
				if( vm.HasError() )
					return Value();

				Value nextValue = vm.CallMemberFunction(object, next_value, {});

				if( vm.HasError() )
					return Value();

				if( !nextValue.AsBool() )
					return false;
			}

			return true;
		}
		else if( argsSize == 2 )
		{
			Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'all(iterable, function)': second argument is not a function");
				return Value();
			}

			while( vm.CallMemberFunction(object, has_value, {}).AsBool() )
			{
				if( vm.HasError() )
					return Value();

				Value nextValue = vm.CallMemberFunction(object, next_value, {});

				if( vm.HasError() )
					return Value();

				Value result = vm.CallFunction(function, {nextValue});

				if( vm.HasError() )
					return Value();

				if( !result.AsBool() )
					return false;
			}

			return true;
		}
		else
		{
			vm.SetError("function 'all' takes one or two arguments");
			return Value();
		}
	}
	else
	{
		vm.SetError("function 'all(iterable)': iterable must be array, object or native-generator");
		return Value();
	}
	
	return Value();
}

Value Any(VirtualMachine& vm, std::vector<Value>& args)
{
	unsigned argsSize = args.size();

	if( argsSize < 1 )
	{
		vm.SetError("function 'any' takes one or two arguments");
		return Value();
	}

	if( args[0].IsArray() )
	{
		auto& array = args[0].array->elements;

		if( argsSize == 1 )
		{
			for( const Value& element : array )
				if( element.AsBool() )
					return true;

			return false;
		}
		else if( argsSize == 2 )
		{
			Value result;
			Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'any(iterable, function)': second argument is not a function");
				return Value();
			}

			for( const Value& element : array )
			{
				result = vm.CallFunction(function, { element });

				if( vm.HasError() )
					return Value();

				if( result.AsBool() )
					return true;
			}

			return false;
		}
		else
		{
			vm.SetError("function 'any' takes one or two arguments");
			return Value();
		}
	}
	else if( args[0].IsNativeGenerator() )
	{
		Generator* generator = args[0].nativeGenerator;

		if( argsSize == 1 )
		{
			while( generator->implementation->has_value() )
				if( generator->implementation->next_value().AsBool() )
					return true;

			return false;
		}
		else if( argsSize == 2 )
		{
			Value result;
			Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'any(iterable, function)': second argument is not a function");
				return Value();
			}

			while( generator->implementation->has_value() )
			{
				result = vm.CallFunction(function, { generator->implementation->next_value() });

				if( vm.HasError() )
					return Value();

				if( result.AsBool() )
					return true;
			}

			return false;
		}
		else
		{
			vm.SetError("function 'any' takes one or two arguments");
			return Value();
		}
	}
	else if( args[0].IsObject() )
	{
		Value& object = args[0];

		Value has_value = vm.GetMember(object, Symbol::HasValueHash);

		if( !has_value.IsFunction() )
		{
			vm.SetError("function 'any(iterable)': iterable doesn't have 'has_value' function");
			return Value();
		}

		Value next_value = vm.GetMember(object, Symbol::NextValueHash);

		if( !next_value.IsFunction() )
		{
			vm.SetError("function 'any(iterable)': iterable doesn't have 'next_value' function");
			return Value();
		}

		if( argsSize == 1 )
		{
			while( vm.CallMemberFunction(object, has_value, {}).AsBool() )
			{
				if( vm.HasError() )
					return Value();

				Value nextValue = vm.CallMemberFunction(object, next_value, {});

				if( vm.HasError() )
					return Value();

				if( nextValue.AsBool() )
					return true;
			}

			return false;
		}
		else if( argsSize == 2 )
		{
			Value& function = args[1];

			if( !function.IsFunction() )
			{
				vm.SetError("function 'any(iterable, function)': second argument is not a function");
				return Value();
			}

			while( vm.CallMemberFunction(object, has_value, {}).AsBool() )
			{
				if( vm.HasError() )
					return Value();

				Value nextValue = vm.CallMemberFunction(object, next_value, {});

				if( vm.HasError() )
					return Value();

				Value result = vm.CallFunction(function, { nextValue });

				if( vm.HasError() )
					return Value();

				if( result.AsBool() )
					return true;
			}

			return false;
		}
		else
		{
			vm.SetError("function 'any' takes one or two arguments");
			return Value();
		}
	}
	else
	{
		vm.SetError("function 'any(iterable)': iterable must be array, object or native-generator");
		return Value();
	}

	return Value();
}

Value Min(VirtualMachine& vm, std::vector<Value>& args)
{
	vm.SetError("function 'min' is not implemented");
	return Value();
}

Value Max(VirtualMachine& vm, std::vector<Value>& args)
{
	vm.SetError("function 'max' is not implemented");
	return Value();
}

Value Sort(VirtualMachine& vm, std::vector<Value>& args)
{
	vm.SetError("function 'sort' is not implemented");
	return Value();
}

Value Abs(VirtualMachine& vm, std::vector<Value>& args)
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

Value Floor(VirtualMachine& vm, std::vector<Value>& args)
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

Value Ceil(VirtualMachine& vm, std::vector<Value>& args)
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

Value Round(VirtualMachine& vm, std::vector<Value>& args)
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

Value Sqrt(VirtualMachine& vm, std::vector<Value>& args)
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

Value Sin(VirtualMachine& vm, std::vector<Value>& args)
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

Value Cos(VirtualMachine& vm, std::vector<Value>& args)
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

Value Tan(VirtualMachine& vm, std::vector<Value>& args)
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
