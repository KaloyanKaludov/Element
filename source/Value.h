#ifndef _VALUE_H_INCLUDED_
#define _VALUE_H_INCLUDED_

#include <vector>
#include <string>

namespace element
{

struct String;
struct Array;
struct Object;
struct Function;
struct Box;
struct Iterator;
struct GarbageCollected;
struct Error;

class VirtualMachine;


struct Value
{
	enum Type : char
	{
		VT_Nil				= 0,
		VT_Int				= 1,
		VT_Float			= 2,
		VT_Bool				= 3,
		VT_Hash				= 4,
		VT_NativeFunction	= 5,

		VT_String			= 6,
		VT_Function			= 7,
		VT_Array			= 8,
		VT_Object			= 9,
		VT_Box				= 10,
		VT_Iterator			= 11,
		VT_Error			= 12,
	};

	Type type;

	typedef Value (*NativeFunction)(VirtualMachine&, const Value&, const std::vector<Value>&);

	union
	{
		int				integer;
		float			floatingPoint;
		bool			boolean;
		unsigned		hash;
		String*			string;
		Array*			array;
		Object*			object;
		Function*		function;
		Box*			box;
		Iterator*		iterator;
		NativeFunction	nativeFunction;
		Error*			error;

		GarbageCollected* garbageCollected;
	};

	Value();
	Value(int integer);
	Value(float floatingPoint);
	Value(bool boolean);
	Value(unsigned hash);
	Value(String* string);
	Value(Array* array);
	Value(Object* object);
	Value(Function* function);
	Value(Box* box);
	Value(Iterator* iterator);
	Value(NativeFunction nativeFunction);
	Value(Error* error);

	Value(const Value& o);

	bool		IsGarbageCollected() const;
	bool		IsNil() const;
	bool		IsFunction() const;
	bool		IsArray() const;
	bool		IsObject() const;
	bool		IsString() const;
	bool		IsBoolean() const;
	bool		IsNumber() const;
	bool		IsFloat() const;
	bool		IsInt() const;
	bool		IsHash() const;
	bool		IsBox() const;
	bool		IsIterator() const;
	bool		IsError() const;

	int			AsInt() const;
	float		AsFloat() const;
	bool		AsBool() const;
	unsigned	AsHash() const;
	std::string	AsString() const;
};

}

#endif // _VALUE_H_INCLUDED_
