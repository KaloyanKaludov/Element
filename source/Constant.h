#ifndef _CONSTANT_INCLUDED_
#define _CONSTANT_INCLUDED_

#include <string>

namespace element
{

struct CodeObject;


struct Constant
{
	enum Type : int
	{
		CT_Nil,
		CT_Bool,
		CT_Integer,
		CT_Float,
		CT_String,
		CT_CodeObject,
	};
	
	Type type;
	
	union
	{
		bool			boolean;
		int				integer;
		float			floatingPoint;
		std::string*	string;
		CodeObject*		codeObject;
		unsigned		size;
	};
	
	Constant();
	Constant(bool boolean);
	Constant(int integer);
	Constant(float floatingPoint);
	Constant(const std::string& string);
	Constant(CodeObject* codeObject);
	~Constant();
	
	bool Equals(int i) const;
	bool Equals(float f) const;
	bool Equals(const std::string& str) const;
	
	unsigned CalculateSize() const;
	
	char* WriteConstant(char* memoryDestination) const;
};

}

#endif // _CONSTANT_INCLUDED_
