#include "Value.h"

#include "GarbageCollected.h"

namespace element
{

Value::Value()
: type(VT_Nil)
, integer(0)
{
}

Value::Value(int integer)
: type(VT_Int)
, integer(integer)
{
}

Value::Value(float floatingPoint)
: type(VT_Float)
, floatingPoint(floatingPoint)
{
}

Value::Value(bool boolean)
: type(VT_Bool)
, boolean(boolean)
{
}

Value::Value(unsigned hash)
: type(VT_Hash)
, hash(hash)
{
}

Value::Value(String* string)
: type(VT_String)
, string(string)
{
}

Value::Value(Array* array)
: type(VT_Array)
, array(array)
{
}

Value::Value(Object* object)
: type(VT_Object)
, object(object)
{
}

Value::Value(Function* function)
: type(VT_Function)
, function(function)
{
}

Value::Value(Box* box)
: type(VT_Box)
, box(box)
{
}

Value::Value(Generator* nativeGenerator)
: type(VT_NativeGenerator)
, nativeGenerator(nativeGenerator)
{
}

Value::Value(NativeFunction nativeFunction)
: type(VT_NativeFunction)
, nativeFunction(nativeFunction)
{
}

Value::Value(Error* error)
: type(VT_Error)
, error(error)
{
}

Value::Value(const Value& o)
: type(o.type)
, function(o.function)
{
}

bool Value::IsGarbageCollected() const
{
	const bool NotGC =	type < VT_String ||
						(type == VT_String && string->state == GarbageCollected::GC_Static) ||
						(type == VT_Function && function->state == GarbageCollected::GC_Static);
	return ! NotGC;
}

bool Value::IsFunction() const
{
	return type == VT_Function || type == VT_NativeFunction;
}

bool Value::IsArray() const
{
	return type == VT_Array;
}

bool Value::IsObject() const
{
	return type == VT_Object;
}

bool Value::IsString() const
{
	return type == VT_String;
}

bool Value::IsBoolean() const
{
	return type == VT_Bool;
}

bool Value::IsNumber() const
{
	return type == VT_Int || type == VT_Float;
}

bool Value::IsFloat() const
{
	return type == VT_Float;
}

bool Value::IsInt() const
{
	return type == VT_Int;
}

bool Value::IsBox() const
{
	return type == VT_Box;
}

bool Value::IsNativeGenerator() const
{
	return type == VT_NativeGenerator;
}

bool Value::IsError() const
{
	return type == VT_Error;
}

int Value::AsInt() const
{
	return type == VT_Int ? integer : int(floatingPoint);
}

float Value::AsFloat() const
{
	return type == VT_Float ? floatingPoint : float(integer);
}

bool Value::AsBool() const
{
	if( type == VT_Bool )
		return boolean;
	if( type == VT_Nil )
		return false;
	return true;
}

unsigned Value::AsHash() const
{
	return hash;
}

std::string Value::AsString() const
{
	switch( type )
	{
	case VT_Nil:
		return "nil";
	case VT_Int:
		return std::to_string(integer);
	case VT_Float:
		return std::to_string(floatingPoint);
	case VT_Bool:
		return boolean ? "true" : "false";
	case VT_String:
		return string->str;
	case VT_Hash:
		return "<hash>";
	case VT_Function:
		return "<function>";
	case VT_Box:
		return "<box>";
	case VT_NativeGenerator:
		return "<native-generator>";
	case VT_NativeFunction:
		return "<native-function>";
	case VT_Error:
		return error->errorString;

	case VT_Array:
	{
		unsigned size = array->elements.size();

		std::string result = "[";

		if( size > 0 )
		{
			for( unsigned i = 0; i < size - 1; ++i )
			{
				Value& element = array->elements[i];

				if( element.IsArray() )
					result += "<array>,";
				else if( element.IsObject() )
					result += "<object>,";
				else
					result += element.AsString() + ", ";
			}

			Value& element = array->elements[size - 1];

			if( element.IsArray() )
				result += "<array>";
			else if( element.IsObject() )
				result += "<object>";
			else
				result += element.AsString();
		}

		result += "]";

		return result;
	}

	case VT_Object:
	{
		unsigned size = object->members.size();

		std::string result = "[ ";

		if( size > 1 ) // we always have atleast the proto member
		{
			for( unsigned i = 1; i < size - 1; ++i )
			{
				auto& kvp = object->members[i];
				if( kvp.value.IsArray() )
					result += std::to_string(kvp.hash) + " = <array>\n  ";
				else if( kvp.value.IsObject() )
					result += std::to_string(kvp.hash) + " = <object>\n  ";
				else
					result += std::to_string(kvp.hash) + " = " + kvp.value.AsString() + "\n  ";
			}

			auto& kvp = object->members[size - 1];
			if( kvp.value.IsArray() )
				result += std::to_string(kvp.hash) + " = <array>\n";
			else if( kvp.value.IsObject() )
				result += std::to_string(kvp.hash) + " = <object>\n";
			else
				result += std::to_string(kvp.hash) + " = " + kvp.value.AsString() + "\n";
		}
		else
		{
			result += "=";
		}

		result += "]";

		return result;
	}
	}

	return "<[???]>";
}

}
