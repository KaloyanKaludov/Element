#ifndef _GARBAGE_COLLECTED_INCLUDED_
#define _GARBAGE_COLLECTED_INCLUDED_

#include <vector>
#include <deque>
#include <string>

#include "Value.h"

namespace element
{

struct CodeObject;
struct ExecutionContext;


struct GarbageCollected
{
	enum State : char // Tri-color marking (incremental garbage collection)
	{
		GC_White0	= 0, // not used 0
		GC_White1	= 1, // not used 1
		GC_Gray		= 2, // to be checked
		GC_Black	= 3, // used

		GC_Static	= 4, // not part of garbage collection (constants)
	};

	GarbageCollected*	next;
	Value::Type			type;
	State				state;

	GarbageCollected(Value::Type type);
};


struct String : public GarbageCollected
{
	std::string str;

	String();
	String(const std::string& str);
	String(const char* data, unsigned size);
};


struct Error : public GarbageCollected
{
	std::string errorString;

	Error();
	Error(const std::string& str);
	Error(const char* data, unsigned size);
};


struct Array : public GarbageCollected
{
	std::vector<Value> elements;

	Array();
};


struct Object : public GarbageCollected
{
	struct Member
	{
		unsigned	hash;
		Value		value;

		Member();
		explicit Member(unsigned hash);
		Member(unsigned hash, const Value& value);
		bool operator<(const Member& o) const;
		bool operator==(const Member& o) const;
	};

	std::vector<Member> members;

	Object();
};


struct Box : public GarbageCollected
{
	Value value;

	Box();
};


struct Function : public GarbageCollected
{
	const CodeObject*	codeObject;
	std::vector<Box*>	freeVariables;
	ExecutionContext*	executionContext;

	Function(const CodeObject* codeObject);
	Function(const Function* o);
};


struct IteratorImplementation
{
	virtual ~IteratorImplementation() = default;
	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) {};
	Value thisObjectUsed;
	Value hasNextFunction;
	Value getNextFunction;
};


struct Iterator : public GarbageCollected
{
	Iterator(IteratorImplementation* implementation);
	~Iterator();

	// The implementation has virtual methods and thus creates virtual tables.
	// For this reason it cannot be a derived struct of the 'Iterator' struct
	// because it will mess up the expected memory layout.
	IteratorImplementation* implementation;
};


struct ArrayIterator : public IteratorImplementation
{
	Array* array;

	unsigned currentIndex = 0;

	ArrayIterator(Array* array);

	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) override;
};


struct StringIterator : public IteratorImplementation
{
	String* str;

	unsigned currentIndex = 0;

	StringIterator(String* str);

	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) override;
};


struct ObjectIterator : public IteratorImplementation
{
	ObjectIterator(const Value& object, const Value& hasNext, const Value& getNext);

	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) override;
};


struct CoroutineIterator : public IteratorImplementation
{
	CoroutineIterator(Function* coroutine);

	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) override;
};

}

#endif // _GARBAGE_COLLECTED_INCLUDED_
