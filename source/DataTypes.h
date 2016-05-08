#ifndef _DATA_TYPES_INCLUDED_
#define _DATA_TYPES_INCLUDED_

#include <vector>
#include <deque>
#include <string>

#include "OpCodes.h"
#include "Value.h"

namespace element
{

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


struct CodeObject
{
	std::vector<AskedVariable> closureMapping;
	std::vector<Instruction> instructions;
	int localVariablesCount;
	int namedParametersCount;
	
	std::vector< std::pair<int, int> > instructionLines;
	
	CodeObject(	Instruction* data, unsigned size,
				std::pair<int, int>* lines, unsigned linesSize,
				int localVariablesCount, int namedParametersCount);
	
	CodeObject(	Instruction* instructions, unsigned instructionsSize, 
				AskedVariable* closure, unsigned closureSize,
				std::pair<int, int>* lines, unsigned linesSize,
				int localVariablesCount, int namedParametersCount);
};


struct Box : public GarbageCollected
{
	Value value;
	
	Box();
};


struct Function : public GarbageCollected
{
	const Instruction* instructions;
	const Instruction* instructionsEnd;
	
	int localVariablesCount;
	int namedParametersCount;
	
	std::vector< std::pair<int, int> >* instructionLines;
	
	std::vector<AskedVariable>*	closureMapping;
	std::vector<Box*>			boxes;
	
	Function(CodeObject* codeObject);
	Function(const Function* o);
};


struct GeneratorImplementation
{
	virtual ~GeneratorImplementation() = default;
	virtual bool has_value() = 0;
	virtual Value next_value() = 0;
	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) = 0;
};


struct Generator : public GarbageCollected
{
	Generator(GeneratorImplementation* implementation);
	~Generator();
	
	GeneratorImplementation* implementation;
};


struct ArrayGenerator : public GeneratorImplementation
{
	Array* array;
	
	unsigned currentIndex = 0;
	
	ArrayGenerator(Array* array);
	
	virtual bool has_value() override;
	virtual Value next_value() override;
	
	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) override;
};

struct StringGenerator : public GeneratorImplementation
{
	String* str;
	
	unsigned currentIndex = 0;
	
	StringGenerator(String* str);
	
	virtual bool has_value() override;
	virtual Value next_value() override;
	
	virtual void UpdateGrayList(std::deque<GarbageCollected*>& grayList,
								GarbageCollected::State currentWhite) override;
};

}

#endif // _DATA_TYPES_INCLUDED_