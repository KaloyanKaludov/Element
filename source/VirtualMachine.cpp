#include "VirtualMachine.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <stdarg.h>
#include "Constant.h"
#include "Symbol.h"
#include "Logger.h"


namespace element
{

VirtualMachine::VirtualMachine(Logger& logger)
: mLogger(logger)
, mLastObject(nullptr)
{
}

void VirtualMachine::Execute(const char* bytecode)
{
	int firstFunctionConstantIndex = ParseBytecode(bytecode);
	
	Function* main = mConstants[ firstFunctionConstantIndex ].function;
	
	mStack.push_back(main);
	
	Call(0);
	
	if( HasError() )
	{
		mStack.clear();
	}
	else
	{
		while( ! mStack.empty() )
		{
			printf("%s", mStack.back().AsString().c_str());
			mStack.pop_back();
			printf("\n");
		}
	}
	
	mStackFrames.clear(); // TODO: obsolete?

	GarbageCollect();
}

int VirtualMachine::AddNativeFunction(Value::NativeFunction function)
{
	mNativeFunctions.push_back(function);
	return int(mNativeFunctions.size()) - 1;
}

MemoryManager& VirtualMachine::GetMemoryManager()
{
	return mMemoryManager;
}

void VirtualMachine::GarbageCollect(int steps)
{
	if( ! mMemoryManager.IsGarbageCollecting() )
	{
		mMemoryManager.ClearGrayList();
		AddRootsToGrayList();
	}

	mMemoryManager.GarbageCollect(steps);
}

void VirtualMachine::SetError(const char* format, ...)
{
	mStackFrames.back().errorState = ES_Error;
	
	char buffer[256];
	int charsWritten = 0;
	
	va_list args;
	va_start(args, format);
	charsWritten += vsprintf(buffer, format, args);
	va_end(args);
	
	buffer[charsWritten++] = '\n';
	buffer[charsWritten] = '\0';
	
	mErrorMessage = std::string(buffer, charsWritten);
}

bool VirtualMachine::HasError() const
{
	return ! mErrorMessage.empty();
}

void VirtualMachine::ClearError()
{
	mErrorMessage.clear();
}

unsigned VirtualMachine::GetHashFromName(const std::string& name)
{
	unsigned hash = Symbol::Hash(name);
	unsigned step = Symbol::HashStep(hash);

	if( name == "proto" )
		hash = Symbol::ProtoHash;

	auto it = mSymbolInfos.find(hash);

	while(	it != mSymbolInfos.end() &&	// found the hash but
			it->second.name != name )	// didn't find the name
	{
		hash += step;
		it = mSymbolInfos.find(hash);
	}

	if( it == mSymbolInfos.end() )
		mSymbolInfos[hash] = {name, -1};

	return hash;
}

int VirtualMachine::GetGlobalIndex(const std::string& name) const
{
	unsigned hash = Symbol::Hash(name);
	unsigned step = Symbol::HashStep(hash);

	if( name == "proto" )
		hash = Symbol::ProtoHash;
	
	auto it = mSymbolInfos.find( hash );
	
	while(	it != mSymbolInfos.end() &&	// found the hash but
			it->second.name != name )	// didn't find the name
	{
		hash += step;
		it = mSymbolInfos.find(hash);
	}
	
	if( it == mSymbolInfos.end() )
		return -1;
	
	return it->second.globalIndex;
}

Value VirtualMachine::GetGlobal(int index) const
{
	if( index >= 0 && index < int(mGlobals.size()) )
		return mGlobals[index];

	return Value();
}

Value VirtualMachine::GetMember(Value& object, const std::string& memberName)
{
	Value result;
	LoadMemberFromObject(object.object, GetHashFromName(memberName), &result);
	return result;
}

Value VirtualMachine::GetMember(Value& object, unsigned memberHash) const
{
	Value result;
	LoadMemberFromObject(object.object, memberHash, &result);
	return result;
}

void VirtualMachine::SetMember(Value& object, unsigned memberHash, const Value& value)
{
	StoreMemberInObject(object.object, memberHash, &value);
}

Value VirtualMachine::CallFunction(const std::string& name, const std::vector<Value>& args)
{
	int index = GetGlobalIndex(name);
	
	if( index < 0 ) // not found
		return Value();
	
	return CallFunction(GetGlobal(index), args);
}

Value VirtualMachine::CallFunction(int index, const std::vector<Value>& args)
{
	return CallFunction(GetGlobal(index), args);
}

Value VirtualMachine::CallFunction(const Value& function, const std::vector<Value>& args)
{
	for( const auto& arg : args )
		mStack.push_back(arg);
	
	mStack.push_back( function );
	
	if( function.type == Value::VT_NativeFunction )
	{
		CallNative( int(args.size()) );
	}
	else // normal function
	{
		Call( int(args.size()) );
	}
	
	Value result = mStack.back();
	
	mStack.pop_back();
	
	return result;
}

Value VirtualMachine::CallMemberFunction(Value& object, const std::string& memberFunctionName, const std::vector<Value>& args)
{
	Value memberFunction = GetMember(object, memberFunctionName);

	return CallMemberFunction(object, memberFunction, args);
}

Value VirtualMachine::CallMemberFunction(Value& object, unsigned functionHash, const std::vector<Value>& args)
{
	Value memberFunction = GetMember(object, functionHash);

	return CallMemberFunction(object, memberFunction, args);
}

Value VirtualMachine::CallMemberFunction(const Value& object, const Value& function, const std::vector<Value>& args)
{
	for( const auto& arg : args )
		mStack.push_back(arg);

	mStack.push_back(function);

	if( function.type == Value::VT_NativeFunction )
	{
		CallNative(int(args.size()));
	}
	else // normal function
	{
		mLastObject = object.object;
		
		Call(int(args.size()));
	}

	Value result = mStack.back();

	mStack.pop_back();

	return result;
}

int VirtualMachine::ParseBytecode(const char* bytecode)
{
	int firstFunctionConstantIndex = -1;
	
	unsigned* p = (unsigned*)bytecode;
	
	unsigned symbolsSize	= *p;
	++p;
	//unsigned symbolsCount	= *p;
	++p;
	//unsigned symbolsOffset= *p;
	++p;
	
	char* symbols	= (char*)p;
	char* symbolsEnd= symbols + symbolsSize;
	
	Symbol*	symbol	= (Symbol*)symbols;
	
	Symbol currentSymbol;
	while( (char*)symbol < symbolsEnd )
	{
		symbol = (Symbol*)currentSymbol.ReadSymbol((char*)symbol);
		mSymbolInfos[currentSymbol.hash] = {currentSymbol.name, currentSymbol.globalIndex};
	}
	
	p = (unsigned*)symbolsEnd;
	
	unsigned constantsSize	= *p;
	++p;
	unsigned constantsCount	= *p;
	++p;
	unsigned constantsOffset= *p;
	++p;
	
	const char*	constants	= (const char*)p;
	const char*	constantsEnd= constants + constantsSize;
	
	Constant* constant = (Constant*)constants;
	
	mConstants.reserve(constantsCount + constantsOffset);
	
	while( (char*)constant < constantsEnd )
	{
		switch(constant->type)
		{
		case Constant::CT_Nil:
			mConstants.emplace_back();
			++constant;
			break;
		
		case Constant::CT_Integer:
			mConstants.emplace_back( constant->integer );
			++constant;
			break;
		
		case Constant::CT_Float:
			mConstants.emplace_back( constant->floatingPoint );
			++constant;
			break;
		
		case Constant::CT_Bool:
			mConstants.emplace_back( constant->boolean );
			++constant;
			break;
		
		case Constant::CT_String:
		{
			unsigned* uintp = (unsigned*)((Constant::Type*)constant + 1);
			unsigned size = *uintp;
			char* buffer = (char*)(uintp + 1);
			
			mConstantStrings.emplace_back(buffer, size);
			
			mConstantStrings.back().state = GarbageCollected::GC_Static;
			
			mConstants.emplace_back( &mConstantStrings.back() );
			
			constant = (Constant*)(buffer + size);
			break;
		}
		
		case Constant::CT_CodeObject:
		{
			unsigned* uintp = (unsigned*)((Constant::Type*)constant + 1);
			unsigned closureSize = *uintp;
			++uintp;
			unsigned instructionsSize = *uintp;
			++uintp;
			unsigned linesSize = *uintp;
			
			int* intp = (int*)(uintp + 1);
			int localVariablesCount = *intp;
			++intp;
			int namedParametersCount = *intp;
			++intp;
			
			int*			closureMapping	= nullptr;
			Instruction*	instructions	= nullptr;
			SourceCodeLine*	lines			= nullptr;
			
			if( closureSize > 0 )
			{
				closureMapping = intp;
				instructions = (Instruction*)(closureMapping + closureSize);
			}
			else // no closure, just instructions
			{
				instructions = (Instruction*)intp;
			}
			
			lines = (SourceCodeLine*)(instructions + instructionsSize);
			
			mConstantCodeObjects.emplace_back(	instructions,
												instructionsSize,
												lines,
												linesSize,
												localVariablesCount,
												namedParametersCount );
			
			CodeObject* codeObject = &mConstantCodeObjects.back();
			
			if( closureSize > 0 )
				codeObject->closureMapping.assign(closureMapping, closureMapping + closureSize);
			
			mConstantFunctions.emplace_back( codeObject );
			
			mConstantFunctions.back().state = GarbageCollected::GC_Static;
			
			mConstants.emplace_back( &mConstantFunctions.back() );
			
			if( firstFunctionConstantIndex == -1 )
				firstFunctionConstantIndex = int(mConstants.size() - 1);
			
			constant = (Constant*)(lines + linesSize);
			break;
		}
		
		default:
			printf("unknown constant type\n");
			++constant;
			break;
		}
	}
	
	return firstFunctionConstantIndex;
}

void VirtualMachine::RunCodeForFrame(Frame* frame)
{
	while( true )
	{
		switch( frame->ip->opCode )
		{
		case OC_Pop: // pop TOS
			mStack.pop_back();
			++frame->ip;
			break;
		
		case OC_PopN: // pop A values from the stack
			for( int i = frame->ip->A; i > 0; --i )
				mStack.pop_back();
			++frame->ip;
			break;
		
		case OC_Rotate2: // swap TOS and TOS1
		{
			int tos = int(mStack.size()) - 1;
			Value value = mStack[tos - 1];
			mStack[tos - 1] = mStack[tos];
			mStack[tos] = value;
			++frame->ip;
			break;
		}
		
		case OC_MoveToTOS2: // copy TOS over TOS2 and pop TOS
		{
			int tos = int(mStack.size()) - 1;
			mStack[tos - 2] = mStack[tos];
			mStack.pop_back();
			++frame->ip;
			break;
		}
				
		case OC_LoadConstant: // A is the index in the constants vector
			mStack.push_back( mConstants[ frame->ip->A ] );
			++frame->ip;
			break;
		
		case OC_LoadLocal: // A is the index in the function scope
			mStack.push_back( frame->variables[ frame->ip->A ] );
			++frame->ip;
			break;
		
		case OC_LoadGlobal: // A is the index in the global scope
		{
			unsigned index = unsigned(frame->ip->A);
			mStack.push_back( index < mGlobals.size() ? mGlobals[index] : Value() );
			++frame->ip;
			break;
		}
		
		case OC_LoadNative: // A is the index in the native functions
			mStack.push_back( mNativeFunctions[ frame->ip->A ] );
			++frame->ip;
			break;
		
		case OC_LoadArgument: // A is the index in the arguments array
			if( int(frame->anonymousParameters.elements.size()) > frame->ip->A )
				mStack.push_back( frame->anonymousParameters.elements[ frame->ip->A ] );
			else
				mStack.emplace_back();
			++frame->ip;
			break;
		
		case OC_LoadArgsArray: // load the current frame's arguments array
			mStack.emplace_back();
			mStack.back().type = Value::VT_Array;
			mStack.back().array = &frame->anonymousParameters;
			++frame->ip;
			break;
		
		case OC_LoadThis: // load the current frame's this object
			mStack.emplace_back( frame->thisObject ? frame->thisObject : Value() );
			++frame->ip;
			break;
		
		case OC_StoreLocal: // A is the index in the function scope
			frame->variables[ frame->ip->A ] = mStack.back();
			++frame->ip;
			break;
		
		case OC_StoreGlobal: // A is the index in the global scope
		{
			unsigned index = unsigned(frame->ip->A);
			if( index >= mGlobals.size() )
				mGlobals.resize(index + 1);
			mGlobals[index] = mStack.back();
			++frame->ip;
			break;
		}
		
		case OC_PopStoreLocal: // A is the index in the function scope
			frame->variables[ frame->ip->A ] = mStack.back();
			mStack.pop_back();
			++frame->ip;
			break;
		
		case OC_PopStoreGlobal: // A is the index in the global scope
		{
			unsigned index = unsigned(frame->ip->A);
			if( index >= mGlobals.size() )
				mGlobals.resize(index + 1);
			mGlobals[index] = mStack.back();
			mStack.pop_back();
			++frame->ip;
			break;
		}
		
		case OC_MakeArray: // A is number of elements to be taken from the stack
		{
			int elementsCount = frame->ip->A;
			
			Array* array = mMemoryManager.NewArray();
			array->elements.resize(elementsCount);
			
			for( int i = elementsCount - 1; i >= 0; --i )
			{
				array->elements[i] = mStack.back();
				mStack.pop_back();
			}
			
			mStack.emplace_back(array);
			
			++frame->ip;
			break;
		}
		
		case OC_LoadElement: // TOS is the index in the TOS1 array
		{
			if( ! mStack.back().IsInt() )
			{
				SetError("Array index must be an integer");
				return;
			}
			
			int index = mStack.back().AsInt();
			mStack.pop_back();
			
			if( ! mStack.back().IsArray() )
			{
				SetError("Attempt to index a non-array value");
				return;
			}
			
			const Array* array = mStack.back().array;
			mStack.pop_back();
			mStack.emplace_back(); // the value to get
			LoadElementFromArray(array, index, &mStack.back());
			++frame->ip;
			break;
		}
		
		case OC_StoreElement: // TOS index, TOS1 array, TOS2 new value
		{
			if( ! mStack.back().IsInt() )
			{
				SetError("Array index must be an integer");
				return;
			}
			
			int index = mStack.back().AsInt();
			mStack.pop_back();
			
			if( ! mStack.back().IsArray() )
			{
				SetError("Attempt to index a non-array value");
				return;
			}
			
			Array* array = mStack.back().array;
			mStack.pop_back();
			StoreElementInArray(array, index, &mStack.back());
			++frame->ip;
			break;
		}
		
		case OC_PopStoreElement: // TOS index, TOS1 array, TOS2 new value
		{
			if( ! mStack.back().IsInt() )
			{
				SetError("Array index must be an integer");
				return;
			}
			
			int index = mStack.back().AsInt();
			mStack.pop_back();
			
			if( ! mStack.back().IsArray() )
			{
				SetError("Attempt to index a non-array value");
				return;
			}
			
			Array* array = mStack.back().array;
			mStack.pop_back();
			StoreElementInArray(array, index, &mStack.back());
			mStack.pop_back();
			++frame->ip;
			break;
		}
		
		case OC_ArrayPushBack:
		{
			Value newValue = mStack.back();
			mStack.pop_back();
			
			if( mStack.back().IsArray() )
			{
				mStack.back().array->elements.push_back( newValue );
				mStack.pop_back();
				mStack.push_back( newValue );
			}
			else
			{
				SetError("Invalid arguments for operator <<");
				return;
			}
			
			++frame->ip;
			break;
		}

		case OC_ArrayPopBack:
		{
			if( mStack.back().IsArray() )
			{
				auto& elements = mStack.back().array->elements;
				if( ! elements.empty() )
				{
					Value popped = elements.back();
					elements.pop_back();
					mStack.pop_back();
					mStack.push_back( popped );
				}
				else
				{
					SetError("Popping from an empty array"); // TODO: non-fatal error?
					return;
				}
			}
			else
			{
				SetError("Invalid arguments for operator >>");
				return;
			}
			
			++frame->ip;
			break;
		}
				
		case OC_MakeObject: // A is number of key-value pairs to be taken from the stack
		{
			int membersCount = frame->ip->A;
			
			Object* object = mMemoryManager.NewObject();
			object->members.resize(membersCount);
						
			for( int i = membersCount - 1; i >= 0; --i )
			{
				Object::Member& member = object->members[i];
				
				member.value = mStack.back();
				mStack.pop_back();
				
				member.hash = mStack.back().AsHash();
				mStack.pop_back();
			}
			
			std::sort(object->members.begin(), object->members.end());
			
			mStack.emplace_back(object);
			
			++frame->ip;
			break;
		}
		
		case OC_MakeEmptyObject: // make an object with just the proto member value
		{
			Object* object = mMemoryManager.NewObject();
			object->members.resize(1);
			
			object->members[0].hash = Symbol::ProtoHash;
			
			mStack.emplace_back(object);
			
			++frame->ip;
			break;
		}
		
		case OC_LoadHash: // H is the hash to load on the stack
			mStack.emplace_back( frame->ip->H );
			++frame->ip;
			break;
		
		case OC_LoadMember: // TOS is the member hash in the TOS1 object
		{
			unsigned hash = mStack.back().AsHash();
			mStack.pop_back();
			
			if( ! mStack.back().IsObject() )
			{
				SetError("Attempt to access a member of a non-object value");
				return;
			}
			
			mLastObject = mStack.back().object;
			mStack.pop_back();
			mStack.emplace_back(); // the value to get
			LoadMemberFromObject(mLastObject, hash, &mStack.back());
			++frame->ip;
			break;
		}
		
		case OC_StoreMember: // TOS member hash, TOS1 object, TOS2 new value
		{
			unsigned hash = mStack.back().AsHash();
			mStack.pop_back();
			
			if( ! mStack.back().IsObject() )
			{
				SetError("Attempt to access a member of a non-object value");
				return;
			}
			
			Object* object = mStack.back().object;
			mStack.pop_back();
			StoreMemberInObject(object, hash, &mStack.back());
			++frame->ip;
			break;
		}
		
		case OC_PopStoreMember: // TOS member hash, TOS1 object, TOS2 new value
		{
			unsigned hash = mStack.back().AsHash();
			mStack.pop_back();
			
			if( ! mStack.back().IsObject() )
			{
				SetError("Attempt to access a member of a non-object value");
				return;
			}
			
			Object* object = mStack.back().object;
			mStack.pop_back();
			StoreMemberInObject(object, hash, &mStack.back());
			mStack.pop_back();
			++frame->ip;
			break;
		}
		
		case OC_MakeGenerator: // make a generator object from TOS and replace it at TOS
		{
			Value::Type type = mStack.back().type;
			if( type == Value::VT_Array )
			{
				Generator* generator = mMemoryManager.NewGeneratorArray( mStack.back().array );
				mStack.pop_back();
				mStack.emplace_back(generator);
			}
			else if( type == Value::VT_String )
			{
				Generator* generator = mMemoryManager.NewGeneratorString( mStack.back().string );
				mStack.pop_back();
				mStack.emplace_back(generator);
			}
			else if( type == Value::VT_NativeGenerator )
			{
				// everything should be OK...
			}
			else if( ! mStack.back().IsObject() )
			{
				SetError("A generator value must be an object");
				return;
			}
			// else lets hope its a proper generator object
			++frame->ip;
			break;
		}
		
		case OC_GeneratorHasValue: // call 'has_value' from the TOS object
		{
			if( mStack.back().IsNativeGenerator() )
			{
				mStack.emplace_back( mStack.back().nativeGenerator->implementation->has_value() );
				++frame->ip;
			}
			else // user defined generator object
			{
				mLastObject = mStack.back().object;
				mStack.emplace_back();
				LoadMemberFromObject(mLastObject, Symbol::HasValueHash, &mStack.back());
				
				if( ! mStack.back().IsFunction() )
				{
					SetError("No 'has_value' function found in generator object");
					return;
				}
				
				if( mStack.back().type == Value::VT_NativeFunction )
				{
					CallNative(0);
				}
				else // normal function
				{
					Call(0);
				}
				
				if( HasError() )
					return;
				
				++frame->ip;
			}
			break;
		}
		
		case OC_GeneratorNextValue: // call 'next_value' from the TOS object
		{
			if( mStack.back().IsNativeGenerator() )
			{
				mStack.emplace_back(mStack.back().nativeGenerator->implementation->next_value());
				++frame->ip;
			}
			else // user defined generator object
			{
				mLastObject = mStack.back().object;
				mStack.emplace_back();
				LoadMemberFromObject(mLastObject, Symbol::NextValueHash, &mStack.back());
				
				if( ! mStack.back().IsFunction() )
				{
					SetError("No 'next_value' function found in generator object");
					return;
				}
				
				if( mStack.back().type == Value::VT_NativeFunction )
				{
					CallNative(0);
				}
				else // normal function
				{
					Call(0);
				}
				
				if( HasError() )
					return;
				
				++frame->ip;
			}
			break;
		}
		
		case OC_MakeBox: // A is the index of the box that needs to be created
		{
			Value& variable = frame->variables[ frame->ip->A ];
			variable = mMemoryManager.NewBox( variable );
			++frame->ip;
			break;
		}
		
		case OC_LoadFromBox: // load the value stored in the box at index A
			mStack.emplace_back( frame->variables[ frame->ip->A ].box->value );
			++frame->ip;
			break;
		
		case OC_StoreToBox: // A is the index of the box that holds the value
		{
			Box* box = frame->variables[ frame->ip->A ].box;
			Value& newValue = mStack.back();
			
			box->value = newValue;
			
			// the tri-color invariant states that at no point shall
			// a black node be directly connected to a white node
			if( box->state == GarbageCollected::GC_Black &&
				newValue.IsGarbageCollected() &&
				newValue.garbageCollected->state == mMemoryManager.GetCurrentWhite() )
			{
				mMemoryManager.MakeGray(newValue.garbageCollected);
			}
			
			++frame->ip;
			break;
		}
		
		case OC_PopStoreToBox: // A is the index of the box that holds the value
		{
			Box* box = frame->variables[ frame->ip->A ].box;
			Value& newValue = mStack.back();
			
			box->value = newValue;
			
			// the tri-color invariant states that at no point shall
			// a black node be directly connected to a white node
			if( box->state == GarbageCollected::GC_Black &&
				newValue.IsGarbageCollected() &&
				newValue.garbageCollected->state == mMemoryManager.GetCurrentWhite() )
			{
				mMemoryManager.MakeGray(newValue.garbageCollected);
			}
			
			mStack.pop_back();
			++frame->ip;
			break;
		}
		
		case OC_MakeClosure: // Create a closure from the function object at TOS and replace it
		{
			Function* newFunction = mMemoryManager.NewFunction( mStack.back().function );
			
			const std::vector<int>& closureMapping = newFunction->codeObject->closureMapping;
			
			newFunction->freeVariables.reserve( closureMapping.size() );
			
			for( int indexToBox : closureMapping )
			{
				if( indexToBox >= 0 )
					newFunction->freeVariables.push_back( frame->variables[ indexToBox ].box );
				else // from a free variable
					newFunction->freeVariables.push_back( frame->function->freeVariables[ -indexToBox - 1 ] );
			}
			
			mStack.back() = Value(newFunction);
			
			++frame->ip;
			break;
		}
		
		case OC_LoadFromClosure: // load the value of the free variable inside the closure at index A
			mStack.emplace_back( frame->function->freeVariables[ frame->ip->A ]->value );
			++frame->ip;
			break;
			
		case OC_StoreToClosure: // A is the index of the free variable inside the closure
		{
			Box* box = frame->function->freeVariables[ frame->ip->A ];
			Value& newValue = mStack.back();
			
			box->value = newValue;
			
			// the tri-color invariant states that at no point shall
			// a black node be directly connected to a white node
			if( box->state == GarbageCollected::GC_Black &&
				newValue.IsGarbageCollected() &&
				newValue.garbageCollected->state == mMemoryManager.GetCurrentWhite() )
			{
				mMemoryManager.MakeGray(newValue.garbageCollected);
			}
			
			++frame->ip;
			break;
		}
		
		case OC_PopStoreToClosure: // A is the index of the free variable inside the closure
		{
			Box* box = frame->function->freeVariables[ frame->ip->A ];
			Value& newValue = mStack.back();
			
			box->value = newValue;
			
			// the tri-color invariant states that at no point shall
			// a black node be directly connected to a white node
			if( box->state == GarbageCollected::GC_Black &&
				newValue.IsGarbageCollected() &&
				newValue.garbageCollected->state == mMemoryManager.GetCurrentWhite() )
			{
				mMemoryManager.MakeGray(newValue.garbageCollected);
			}
			
			mStack.pop_back();
			++frame->ip;
			break;
		}
		
		case OC_Jump: // jump to A
			frame->ip = &frame->instructions[ frame->ip->A ];
			break;
		
		case OC_JumpIfFalse: // jump to A, if TOS is false
			if( mStack.back().AsBool() )
				++frame->ip;
			else
				frame->ip = &frame->instructions[ frame->ip->A ];
			break;
		
		case OC_PopJumpIfFalse: // jump to A, if TOS is false, pop TOS either way
			if( mStack.back().AsBool() )
				++frame->ip;
			else
				frame->ip = &frame->instructions[ frame->ip->A ];
			mStack.pop_back();
			break;
		
		case OC_JumpIfFalseOrPop: // jump to A, if TOS is false, otherwise pop TOS (and-op)
			if( mStack.back().AsBool() )
			{
				mStack.pop_back();
				++frame->ip;
			}
			else
			{
				frame->ip = &frame->instructions[ frame->ip->A ];
			}
			break;
		
		case OC_JumpIfTrueOrPop: // jump to A, if TOS is true, otherwise pop TOS (or-op)
			if( mStack.back().AsBool() )
			{
				frame->ip = &frame->instructions[ frame->ip->A ];
			}
			else
			{
				mStack.pop_back();
				++frame->ip;
			}
			break;
		
		case OC_FunctionCall: // function to call and arguments are on stack, A is arguments count
			if( ! mStack.back().IsFunction() )
			{
				SetError("Attempt to call a non-function value");
				return;
			}
			
			if( mStack.back().type == Value::VT_NativeFunction )
			{
				CallNative( frame->ip->A );
			}
			else // normal function
			{
				Call( frame->ip->A );
			}
			
			if( HasError() )
				return;
			
			++frame->ip;
			break;
		
		case OC_EndFunction: // end function sentinel
			return;
		
		case OC_Add:
		case OC_Subtract:
		case OC_Multiply:
		case OC_Divide:
		case OC_Power:
		case OC_Modulo:
		case OC_Concatenate:
		case OC_Xor:
		
		case OC_Equal:
		case OC_NotEqual:
		case OC_Less:
		case OC_Greater:
		case OC_LessEqual:
		case OC_GreaterEqual:
			if( ! DoBinaryOperation(frame->ip->opCode) )
				return;
			
			++frame->ip;
			break;
		
		case OC_UnaryPlus:
			if( ! mStack.back().IsNumber() )
			{
				SetError("Unary plus used on a value that is not an integer or float");
				return;
			}
			
			++frame->ip; // do nothing (:
			break;
		
		case OC_UnaryMinus:
			if( mStack.back().IsInt() )
			{
				int i = mStack.back().AsInt();
				mStack.pop_back();
				mStack.emplace_back(-i);
			}
			else if( mStack.back().IsFloat() )
			{
				float f = mStack.back().AsFloat();
				mStack.pop_back();
				mStack.emplace_back(-f);
			}
			else
			{
				SetError("Unary minus used on a value that is not an integer or float");
				return;
			}
			
			++frame->ip;
			break;
			
		case OC_UnaryNot:
		{
			bool b = mStack.back().AsBool(); // anything can be turned into a bool
			mStack.pop_back();
			mStack.emplace_back(!b);
			++frame->ip;
			break;
		}
		
		case OC_UnaryConcatenate:
		{
			std::string str = mStack.back().AsString(); // anything can be turned into a string
			mStack.pop_back();
			mStack.emplace_back( mMemoryManager.NewString(str) );
			++frame->ip;
			break;
		}
		
		case OC_UnarySizeOf:
		{
			const Value& value = mStack.back();
			int size = 0;
			
			if( value.IsArray() )
				size = int(value.array->elements.size());
			else if( value.IsObject() )
				size = int(value.object->members.size());
			else if( value.IsString() )
				size = int(value.string->str.size());
			else
			{
				SetError("Attempt to get the size of a value that is not an array, object or string");
				return;
			}
			
			mStack.pop_back();
			mStack.emplace_back(size);
			
			++frame->ip;
			break;
		}
		
		default:
			SetError("Invalid OpCode!");
			return;
		}
	}
}

void VirtualMachine::Call(int argumentsCount)
{
	Function* function = mStack.back().function;
	mStack.pop_back();
	
	const CodeObject* codeObject = function->codeObject;
	
	// create a new stack frame ////////////////////////////////////////////////
	mStackFrames.emplace_back();
	Frame* newFrame = &mStackFrames.back();
	
	newFrame->function		= function;
	newFrame->instructions	= codeObject->instructions.data();
	newFrame->ip			= newFrame->instructions;
	newFrame->thisObject	= mLastObject;
	
	newFrame->variables.resize( codeObject->localVariablesCount );
	
	// bind parameters to local variables //////////////////////////////////////
	int namedParametersCount = codeObject->namedParametersCount;
	int anonymousArgumentsCount = argumentsCount - namedParametersCount;
	
	if( anonymousArgumentsCount <= 0 )
	{
		for( int i = argumentsCount - 1; i >= 0; --i )
		{
			newFrame->variables[i] = mStack.back();
			mStack.pop_back();
		}
	}
	else // we have some anonymous arguments
	{
		newFrame->anonymousParameters.elements.resize(anonymousArgumentsCount);
		
		for( int i = anonymousArgumentsCount - 1; i >= 0; --i )
		{
			newFrame->anonymousParameters.elements[i] = mStack.back();
			mStack.pop_back();
		}
		
		for( int i = namedParametersCount - 1; i >= 0; --i )
		{
			newFrame->variables[i] = mStack.back();
			mStack.pop_back();
		}
	}
	
	// execute instructions ////////////////////////////////////////////////////
	RunCodeForFrame(newFrame);
	
	// check and log errors ////////////////////////////////////////////////////
	if( newFrame->errorState != ES_NoError )
	{
		bool fromThisFrame = newFrame->errorState == ES_Error;
		int line = CurrentLineFromFrame(newFrame);
		
		mLogger.PushError(line, fromThisFrame ? mErrorMessage.c_str() : "called from here");
		
		unsigned stackSize = mStackFrames.size();

		if( stackSize >= 2 )
			mStackFrames[stackSize - 2].errorState = ES_Propagated;
	}
	
	// restore previous stack frame ////////////////////////////////////////////
	mStackFrames.pop_back();
}

void VirtualMachine::CallNative(int argumentsCount)
{
	Value::NativeFunction function = mStack.back().nativeFunction;
	mStack.pop_back();
	
	std::vector<Value> arguments;
	arguments.resize(argumentsCount);
	
	for( int i = argumentsCount - 1; i >= 0; --i )
	{
		arguments[i] = mStack.back();
		mStack.pop_back();
	}
	
	Value result = function(*this, arguments);
	
	mStack.push_back(result);
}

void VirtualMachine::LoadElementFromArray(const Array* array, int index, Value* outValue) const
{
	int size = int(array->elements.size());

	if( size == 0 )
	{
		*outValue = Value();
		return;
	}

	while( index < 0 )
		index += size;

	*outValue = array->elements[index];
}

void VirtualMachine::StoreElementInArray(Array* array, int index, const Value* newValue)
{
	int size = int(array->elements.size());

	if( size == 0 )
	{
		if( index < 0 )
			index = 0;
	}
	else
	{
		while( index < 0 )
			index += size;
	}
	
	if( index >= size )
		array->elements.resize(index + 1);
	
	array->elements[index] = *newValue;
	
	// the tri-color invariant states that at no point shall
	// a black node be directly connected to a white node
	if( array->state == GarbageCollected::GC_Black &&
		newValue->IsGarbageCollected() &&
		newValue->garbageCollected->state == mMemoryManager.GetCurrentWhite() )
	{
		mMemoryManager.MakeGray(array);
	}
}

void VirtualMachine::LoadMemberFromObject(Object* object, unsigned hash, Value* outValue) const
{
	Object::Member member(hash);
	
	auto it = std::lower_bound(object->members.begin(), object->members.end(), member);
	
	if( it == object->members.end() || it->hash != hash )
	{
		const Value* proto = &object->members[0].value;
		
		while( proto->type == Value::VT_Object ) // it has a proto object
		{
			std::vector<Object::Member>& members = proto->object->members;
			
			auto it = std::lower_bound(members.begin(), members.end(), member);
			
			if( it == members.end() || it->hash != hash )
			{
				proto = &members[0].value;
			}
			else // found in one of the proto objects
			{
				*outValue = it->value;
				return;
			}
		}
		
		// not found, out value shall stay nil
	}
	else // found the value corresponding to this hash
	{
		*outValue = it->value;
	}
}

void VirtualMachine::StoreMemberInObject(Object* object, unsigned hash, const Value* newValue)
{
	Object::Member member(hash);
	
	auto it = std::lower_bound(object->members.begin(), object->members.end(), member);
	
	if( it == object->members.end() || it->hash != hash )
	{
		bool found = false;
		const Value* proto = &object->members[0].value;
		
		while( proto->type == Value::VT_Object ) // it has a proto object
		{
			std::vector<Object::Member>& members = proto->object->members;
			
			auto it = std::lower_bound(members.begin(), members.end(), member);
			
			if( it == members.end() || it->hash != hash )
			{
				proto = &members[0].value;
			}
			else // found in one of the proto objects
			{
				it->value = *newValue;
				found = true;
				break;
			}
		}
		
		if( ! found ) // create a new one
			object->members.insert(it, Object::Member(hash, *newValue));
	}
	else // found the value corresponding to this hash
	{
		it->value = *newValue;
	}
	
	// the tri-color invariant states that at no point shall
	// a black node be directly connected to a white node
	if( object->state == GarbageCollected::GC_Black &&
		newValue->IsGarbageCollected() &&
		newValue->garbageCollected->state == mMemoryManager.GetCurrentWhite() )
	{
		mMemoryManager.MakeGray(object);
	}
}

bool VirtualMachine::DoBinaryOperation(int opCode)
{
	unsigned last = mStack.size() - 1;
	Value& lhs = mStack[last - 1];
	Value& rhs = mStack[last];
	Value result;
	
	switch( opCode )
	{
		case OpCode::OC_Add:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				 if( lhs.IsFloat() || rhs.IsFloat() )
					result = Value( lhs.AsFloat() + rhs.AsFloat() );
				else
					result = Value( lhs.AsInt() + rhs.AsInt() );
			}
			else if( lhs.IsArray() && rhs.IsArray() )
			{
				Array* newArray = mMemoryManager.NewArray();
				auto& elements = newArray->elements;
				elements.reserve(lhs.array->elements.size() + rhs.array->elements.size());

				for( const auto& v : lhs.array->elements )
					elements.push_back(v);
				for( const auto& v : rhs.array->elements )
					elements.push_back(v);

				result = newArray;
			}
			else if( lhs.IsObject() && rhs.IsObject() )
			{
				Object* newObject = mMemoryManager.NewObject();
				auto& members = newObject->members;
				members.reserve(lhs.object->members.size() + rhs.object->members.size());

				for( const auto& v : lhs.object->members )
					members.push_back(v);
				for( const auto& v : rhs.object->members )
					members.push_back(v);

				std::sort(members.begin(), members.end());

				result = newObject;
			}
			else
			{
				SetError("Invalid arguments for operator +");
				return false;
			}
			break;
		}
		
		case OpCode::OC_Subtract:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( lhs.IsFloat() || rhs.IsFloat() )
					result = Value( lhs.AsFloat() - rhs.AsFloat() );
				else
					result = Value( lhs.AsInt() - rhs.AsInt() );
			}
			else
			{
				SetError("Invalid arguments for operator -");
				return false;
			}
			break;
		}
		
		case OpCode::OC_Multiply:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( lhs.IsFloat() || rhs.IsFloat() )
					result = Value( lhs.AsFloat() * rhs.AsFloat() );
				else
					result = Value( lhs.AsInt() * rhs.AsInt() );
			}
			else
			{
				SetError("Invalid arguments for operator *");
				return false;
			}
			break;
		}
		
		case OpCode::OC_Divide:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( rhs.AsFloat() == 0 )
				{
					SetError("Division by 0");
				}
				else if( lhs.IsFloat() || rhs.IsFloat() )
					result = Value( lhs.AsFloat() / rhs.AsFloat() );
				else
					result = Value( lhs.AsInt() / rhs.AsInt() );
			}
			else
			{
				SetError("Invalid arguments for operator /");
				return false;
			}
			break;
		}
		
		case OC_Power:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( lhs.IsFloat() )
					result = float(std::pow(lhs.AsFloat(), rhs.AsFloat()));
				else
					result = int(std::pow(lhs.AsInt(), rhs.AsFloat()));
			}
			else
			{
				SetError("Invalid arguments for operator ^");
				return false;
			}
			break;
		}
		
		case OC_Modulo:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				result = Value( lhs.AsInt() % rhs.AsInt() );
			}
			else
			{
				SetError("Invalid arguments for operator %");
				return false;
			}
			break;
		}
		
		case OC_Concatenate:
		{
			if( lhs.IsString() && rhs.IsString() )
				result = mMemoryManager.NewString(lhs.string->str + rhs.string->str);
			else // anything can be turned into a string
				result = mMemoryManager.NewString(lhs.AsString() + rhs.AsString());
			break;
		}
		
		case OC_Xor:
		{
			result = !lhs.AsBool() != !rhs.AsBool(); // anything can be turned into a bool
			break;
		}
		
		case OC_Equal:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
				result = lhs.AsFloat() == rhs.AsFloat();
			else if( lhs.IsBoolean() && rhs.IsBoolean() )
				result = lhs.AsBool() == rhs.AsBool();
			else if( lhs.IsString() && rhs.IsString() )
				result = lhs.AsString() == rhs.AsString();
			else
				result = lhs.object == rhs.object; // compare any pointers
			break;
		}
		
		case OC_NotEqual:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
				result = lhs.AsFloat() != rhs.AsFloat();
			else if( lhs.IsBoolean() && rhs.IsBoolean() )
				result = lhs.AsBool() != rhs.AsBool();
			else if( lhs.IsString() && rhs.IsString() )
				result = lhs.AsString() != rhs.AsString();
			else
				result = lhs.object != rhs.object; // compare any pointers
			break;
		}
		
		case OC_Less:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( lhs.IsFloat() || rhs.IsFloat() )
					result = lhs.AsFloat() < rhs.AsFloat();
				else
					result = lhs.AsInt() < rhs.AsInt();
			}
			else
			{
				SetError("Invalid arguments for operator <");
				return false;
			}
			break;
		}
		
		case OC_Greater:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( lhs.IsFloat() || rhs.IsFloat() )
					result = lhs.AsFloat() > rhs.AsFloat();
				else
					result = lhs.AsInt() > rhs.AsInt();
			}
			else
			{
				SetError("Invalid arguments for operator >");
				return false;
			}
			break;
		}
		
		case OC_LessEqual:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( lhs.IsFloat() || rhs.IsFloat() )
					result = lhs.AsFloat() <= rhs.AsFloat();
				else
					result = lhs.AsInt() <= rhs.AsInt();
			}
			else
			{
				SetError("Invalid arguments for operator <=");
				return false;
			}
			break;
		}
		
		case OC_GreaterEqual:
		{
			if( lhs.IsNumber() && rhs.IsNumber() )
			{
				if( lhs.IsFloat() || rhs.IsFloat() )
					result = lhs.AsFloat() >= rhs.AsFloat();
				else
					result = lhs.AsInt() >= rhs.AsInt();
			}
			else
			{
				SetError("Invalid arguments for operator >=");
				return false;
			}
			break;
		}
	}
	
	mStack.pop_back();
	mStack.pop_back();
	mStack.push_back(result);
	
	return true;
}

int VirtualMachine::CurrentLineFromFrame(const Frame* frame) const
{
	const auto& lines = frame->function->codeObject->instructionLines;
	
	if( lines.empty() )
		return -1;
	
	if( lines.size() == 1 )
		return lines.back().line;
	
	int instructionIndex = frame->ip - frame->function->codeObject->instructions.data();
	
	int lineIndex = -1;
	
	for( const SourceCodeLine& line : lines )
	{
		if( instructionIndex >= line.instructionIndex )
			lineIndex = line.line;
		else
			return lineIndex;
	}
	
	return lineIndex;
}

void VirtualMachine::AddRootsToGrayList()
{
	for( Value& global : mGlobals )
		if( global.IsGarbageCollected() && global.IsWhite() )
			mMemoryManager.MakeGray(global.garbageCollected);
	
	for( Frame& frame : mStackFrames )
	{
		for( Value& local : frame.variables )
			if( local.IsGarbageCollected() && local.IsWhite() )
				mMemoryManager.MakeGray(local.garbageCollected);
		
		for( Value& anonymousParameter : frame.anonymousParameters.elements )
			if( anonymousParameter.IsGarbageCollected() && anonymousParameter.IsWhite() )
				mMemoryManager.MakeGray(anonymousParameter.garbageCollected);
	}
	
	for( Value& value : mStack )
		if( value.IsGarbageCollected() && value.IsWhite() )
			mMemoryManager.MakeGray(value.garbageCollected);
}

}
