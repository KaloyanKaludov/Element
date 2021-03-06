#include "VirtualMachine.h"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <fstream>
#include "AST.h"
#include "Native.h"


namespace element
{

VirtualMachine::VirtualMachine()
: mLogger()
, mParser(mLogger)
, mSemanticAnalyzer(mLogger)
, mCompiler(mLogger)
, mFileManager()
, mMemoryManager()
, mExecutionContext(nullptr)
, mStack(nullptr)
{
	RegisterStandardUtilities();
}

void VirtualMachine::ResetState()
{
	mLogger.ClearErrorMessages();
	
	mSemanticAnalyzer.ResetState();

	mCompiler.ResetState();

	mFileManager.ResetState();

	mMemoryManager.ResetState();
	
	mConstantStrings.clear();
	mConstantFunctions.clear();
	mConstantCodeObjects.clear();
	mConstants.clear();
	
	mNativeFunctions.clear();
	mSymbolNames.clear();
	
	mExecutionContext = nullptr;
	mStack = nullptr;
	
	mErrorMessage.clear();
	
	RegisterStandardUtilities();
}

Value VirtualMachine::Interpret(std::istream& input)
{
	Value result;

	std::unique_ptr<ast::FunctionNode> node = mParser.Parse(input);

	if( !mLogger.HasErrorMessages() )
	{
		mSemanticAnalyzer.Analyze(node.get());

		if( !mLogger.HasErrorMessages() )
		{
			std::unique_ptr<char[]> bytecode = mCompiler.Compile(node.get());

			if( !mLogger.HasErrorMessages() )
			{
				result = ExecuteBytecode(bytecode.get(), mMemoryManager.GetDefaultModule());
			}
		}
	}

	if( mLogger.HasErrorMessages() )
	{
		result = mMemoryManager.NewError( mLogger.GetCombinedErrorMessages() );
		mLogger.ClearErrorMessages();
	}

	ClearError();

	return result;
}

Value VirtualMachine::Interpret(const std::string& filename)
{
	std::string fileToExecute = mFileManager.PushFileToExecute(filename);
	
	if( fileToExecute.empty() )
		return mMemoryManager.NewError("file-not-found");
	
	Module& module = mMemoryManager.GetModuleForFile(fileToExecute);
	
	if( module.bytecode.get() != nullptr )
	{
		mFileManager.PopFileToExecute();
		return module.result;
	}
	
	Value result;
	std::unique_ptr<char[]> bytecode;
	
	std::ifstream input(fileToExecute);

	std::unique_ptr<ast::FunctionNode> node = mParser.Parse(input);

	if( !mLogger.HasErrorMessages() )
	{
		mSemanticAnalyzer.Analyze(node.get());

		if( !mLogger.HasErrorMessages() )
		{
			bytecode = mCompiler.Compile(node.get());

			if( !mLogger.HasErrorMessages() )
			{
				result = ExecuteBytecode(bytecode.get(), module);
			}
		}
	}

	if( mLogger.HasErrorMessages() )
	{
		result = mMemoryManager.NewError( mLogger.GetCombinedErrorMessages() );
		mLogger.ClearErrorMessages();
	}

	ClearError();
	
	module.result = result;
	module.bytecode = std::move(bytecode);
	
	mFileManager.PopFileToExecute();

	return result;
}

FileManager& VirtualMachine::GetFileManager()
{
	return mFileManager;
}

MemoryManager& VirtualMachine::GetMemoryManager()
{
	return mMemoryManager;
}

void VirtualMachine::SetError(const std::string& errorMessage)
{
	mErrorMessage = errorMessage;
}

bool VirtualMachine::HasError() const
{
	return ! mErrorMessage.empty();
}

void VirtualMachine::ClearError()
{
	mErrorMessage.clear();
}

void VirtualMachine::RegisterNativeFunction(const std::string& name, Value::NativeFunction function)
{
	int index = int(mNativeFunctions.size());
	
	mNativeFunctions.push_back(function);
	
	mSemanticAnalyzer.AddNativeFunction(name, index);
}

std::string VirtualMachine::GetVersion() const
{
	return "element interpreter version 0.0.5";
}

Iterator* VirtualMachine::MakeIteratorForValue(const Value& value)
{
	switch( value.type )
	{
	case Value::VT_Iterator:
		return value.iterator;
	
	case Value::VT_Array:
		return mMemoryManager.NewIterator( new ArrayIterator(value.array) );
	
	case Value::VT_String:
		return mMemoryManager.NewIterator( new StringIterator(value.string) );
	
	case Value::VT_Object:
	{
		Value hasNextMemberFunction;
		LoadMemberFromObject(value.object, Symbol::HasNextHash, &hasNextMemberFunction);
		
		Value getNextMemberFunction;
		LoadMemberFromObject(value.object, Symbol::GetNextHash, &getNextMemberFunction);
		
		if( hasNextMemberFunction.IsNil() || getNextMemberFunction.IsNil() )
			return nullptr;
		
		return mMemoryManager.NewIterator( new ObjectIterator(value, hasNextMemberFunction, getNextMemberFunction) );
	}
	
	case Value::VT_Function:
		if( value.function->executionContext ) // only coroutines
			return mMemoryManager.NewIterator( new CoroutineIterator(value.function) );
	
	default:
		return nullptr;
	}
}

unsigned VirtualMachine::GetHashFromName(const std::string& name)
{
	unsigned hash = Symbol::Hash(name);
	unsigned step = Symbol::HashStep(hash);

	if( name == "proto" )
		hash = Symbol::ProtoHash;

	auto it = mSymbolNames.find(hash);

	while(	it != mSymbolNames.end() &&	// found the hash but
			it->second != name )		// didn't find the name
	{
		hash += step;
		it = mSymbolNames.find(hash);
	}

	if( it == mSymbolNames.end() )
		mSymbolNames[hash] = name;

	return hash;
}

bool VirtualMachine::GetNameFromHash(unsigned hash, std::string* name)
{
	auto it = mSymbolNames.find(hash);
	
	if( it != mSymbolNames.end() )
	{
		if( name )
			*name = it->second;
		return true;
	}
	
	return false;
}

Value VirtualMachine::GetMember(const Value& object, const std::string& memberName)
{
	Value result;
	LoadMemberFromObject(object.object, GetHashFromName(memberName), &result);
	return result;
}

Value VirtualMachine::GetMember(const Value& object, unsigned memberHash) const
{
	Value result;
	LoadMemberFromObject(object.object, memberHash, &result);
	return result;
}

void VirtualMachine::SetMember(const Value& object, const std::string& memberName, const Value& value)
{
	StoreMemberInObject(object.object, GetHashFromName(memberName), value);
}

void VirtualMachine::SetMember(const Value& object, unsigned memberHash, const Value& value)
{
	StoreMemberInObject(object.object, memberHash, value);
}

void VirtualMachine::PushElement(const Value& array, const Value& value)
{
	PushElementToArray(array.array, value);
}

void VirtualMachine::AddElement(const Value& array, int atIndex, const Value& value)
{
	StoreElementInArray(array.array, atIndex, value);
}

Value VirtualMachine::CallFunction(const Value& function, const std::vector<Value>& args)
{
	return CallFunction_Common(Value(), function, args);
}

Value VirtualMachine::CallMemberFunction(const Value& object, const std::string& memberFunctionName, const std::vector<Value>& args)
{
	Value memberFunction = GetMember(object, memberFunctionName);

	return CallFunction_Common(object, memberFunction, args);
}

Value VirtualMachine::CallMemberFunction(const Value& object, unsigned functionHash, const std::vector<Value>& args)
{
	Value memberFunction = GetMember(object, functionHash);

	return CallFunction_Common(object, memberFunction, args);
}

Value VirtualMachine::CallMemberFunction(const Value& object, const Value& function, const std::vector<Value>& args)
{
	return CallFunction_Common(object, function, args);
}

Value VirtualMachine::ExecuteBytecode(const char* bytecode, Module& forModule)
{
	int firstFunctionConstantIndex = ParseBytecode(bytecode, forModule);

	Function* main = mConstants[ firstFunctionConstantIndex ].function;

	ExecutionContext dummyContext;

	if( !mExecutionContext ) // first run
	{
		mExecutionContext = &dummyContext;
		mStack = &dummyContext.stack;
	}
	
	return CallFunction_Common(Value(), main, {});
}

int VirtualMachine::ParseBytecode(const char* bytecode, Module& forModule)
{
	int firstFunctionConstantIndex = -1;

	unsigned* p = (unsigned*)bytecode;

	unsigned symbolsSize = *p;
	++p;
	unsigned symbolsCount = *p;
	++p;
	unsigned symbolsOffset = *p;
	++p;

	char* symbolIt = (char*)p;
	char* symbolsEnd = symbolIt + symbolsSize;

	p = (unsigned*)symbolsEnd;

	unsigned constantsSize = *p;
	++p;
	unsigned constantsCount = *p;
	++p;
	unsigned constantsOffset = *p;
	++p;

	char* constantIt = (char*)p;
	char* constantsEnd = constantIt + constantsSize;

	mSymbolNames.reserve(symbolsCount + symbolsOffset);

	Symbol currentSymbol;

	while( symbolIt < symbolsEnd )
	{
		symbolIt = currentSymbol.ReadSymbol(symbolIt);
		
		mSymbolNames[currentSymbol.hash] = currentSymbol.name;
	}

	mConstants.reserve(constantsCount + constantsOffset);

	Constant currentConstant;

	while( constantIt < constantsEnd )
	{
		constantIt = currentConstant.ReadConstant(constantIt);
		
		switch( currentConstant.type )
		{
		case Constant::CT_Nil:
			mConstants.emplace_back();
			break;
		
		case Constant::CT_Integer:
			mConstants.emplace_back( currentConstant.integer );
			break;

		case Constant::CT_Float:
			mConstants.emplace_back( currentConstant.floatingPoint );
			break;

		case Constant::CT_Bool:
			mConstants.emplace_back( currentConstant.boolean );
			break;
		
		case Constant::CT_String:
		{
			mConstantStrings.emplace_back( std::move(*(currentConstant.string)) );
			
			mConstantStrings.back().state = GarbageCollected::GC_Static;
			
			mConstants.emplace_back( &mConstantStrings.back() );
			break;
		}
		
		case Constant::CT_CodeObject:
		{
			mConstantCodeObjects.emplace_back( std::move(*currentConstant.codeObject) );
			
			CodeObject* codeObject = &mConstantCodeObjects.back();

			codeObject->module = &forModule;

			mConstantFunctions.emplace_back( codeObject );
			mConstantFunctions.back().state = GarbageCollected::GC_Static;
			
			if( firstFunctionConstantIndex == -1 )
				firstFunctionConstantIndex = int(mConstants.size());

			mConstants.emplace_back( &mConstantFunctions.back() );
			break;
		}
		
		default:
			SetError("Unknown constant type");
			return firstFunctionConstantIndex;
		}
	}

	return firstFunctionConstantIndex;
}

Value VirtualMachine::CallFunction_Common(const Value& thisObject, const Value& function, const std::vector<Value>& args)
{
	if( function.type == Value::VT_NativeFunction )
	{
		return function.nativeFunction(*this, thisObject, args);
	}
	else // normal function
	{
		// switch context
		ExecutionContext* oldContext = mExecutionContext;
		mExecutionContext = mMemoryManager.NewRootExecutionContext();
		mStack = &mExecutionContext->stack;

		mStack->reserve(args.size());
		std::copy(args.begin(), args.end(), std::back_inserter(*mStack));
		mStack->push_back( function );

		mExecutionContext->lastObject = thisObject;

		Call( int(args.size()) );

		Value result = RunCode();

		mMemoryManager.DeleteRootExecutionContext(mExecutionContext);
		mExecutionContext = oldContext;
		mStack = &oldContext->stack;

		return result;
	}
}

Value VirtualMachine::RunCode()
{
	StackFrame* frame = nullptr;

	// IMPORTANT: the current 'mExecutionContext' may change during 'RunCodeForFrame'
	while( ! mExecutionContext->stackFrames.empty() )
	{
		frame = &mExecutionContext->stackFrames.back();

		RunCodeForFrame( frame );

		if( HasError() )
		{
			LogStackTraceStartingFrom(frame);

			return mMemoryManager.NewError("runtime-error");
		}
	}
	
	// result if any
	Value result;

	if( ! mStack->empty() )
	{
		result = mStack->back();
		mStack->pop_back();
	}

	return result;
}

void VirtualMachine::RunCodeForFrame(StackFrame* frame)
{
	while( true )
	{
		switch( frame->ip->opCode )
		{
		case OC_Pop: // pop TOS
			mStack->pop_back();
			++frame->ip;
			break;

		case OC_PopN: // pop A values from the stack
			for( int i = frame->ip->A; i > 0; --i )
				mStack->pop_back();
			++frame->ip;
			break;

		case OC_Rotate2: // swap TOS and TOS1
		{
			int tos = int(mStack->size()) - 1;
			Value value = mStack->at(tos - 1);
			mStack->at(tos - 1) = mStack->at(tos);
			mStack->at(tos) = value;
			++frame->ip;
			break;
		}

		case OC_MoveToTOS2: // copy TOS over TOS2 and pop TOS
		{
			int tos = int(mStack->size()) - 1;
			mStack->at(tos - 2) = mStack->at(tos);
			mStack->pop_back();
			++frame->ip;
			break;
		}

		case OC_Duplicate: // make a copy of TOS and push it to the stack
		{
			mStack->push_back( mStack->back() );
			++frame->ip;
			break;
		}

		case OC_Unpack: // A is the number of values to be produced from the TOS value
		{
			Value valueToUnpack = mStack->back();
			mStack->pop_back();

			int expectedSize = frame->ip->A;

			if( valueToUnpack.IsArray() )
			{
				const std::vector<Value>& elements = valueToUnpack.array->elements;
				int arraySize = int(elements.size());

                if( arraySize >= expectedSize )
				{
                    for( int i = expectedSize - 1; i >= 0; --i )
						mStack->push_back(elements[i]);
				}
				else // arraySize < expectedSize
				{
					for( int i = expectedSize - arraySize; i > 0; --i )
						mStack->emplace_back(); // nil

					for( int i = arraySize - 1; i >= 0; --i )
						mStack->push_back(elements[i]);
				}
			}
			else
			{
				for( int i = expectedSize - 1; i > 0; --i )
					mStack->emplace_back(); // nil

				mStack->push_back(valueToUnpack);
			}

			++frame->ip;
			break;
		}

		case OC_LoadConstant: // A is the index in the constants vector
			mStack->push_back( mConstants[ frame->ip->A ] );
			++frame->ip;
			break;

		case OC_LoadLocal: // A is the index in the function scope
			mStack->push_back( frame->variables[ frame->ip->A ] );
			++frame->ip;
			break;

		case OC_LoadGlobal: // A is the index in the global scope
		{
			unsigned index = unsigned(frame->ip->A);
			mStack->push_back( index < frame->globals->size() ? frame->globals->at(index) : Value() );
			++frame->ip;
			break;
		}

		case OC_LoadNative: // A is the index in the native functions
			mStack->push_back( mNativeFunctions[ frame->ip->A ] );
			++frame->ip;
			break;

		case OC_LoadArgument: // A is the index in the arguments array
			if( int(frame->anonymousParameters.elements.size()) > frame->ip->A )
				mStack->push_back( frame->anonymousParameters.elements[ frame->ip->A ] );
			else
				mStack->emplace_back();
			++frame->ip;
			break;

		case OC_LoadArgsArray: // load the current frame's arguments array
			mStack->emplace_back();
			mStack->back().type = Value::VT_Array;
			mStack->back().array = &frame->anonymousParameters;
			++frame->ip;
			break;

		case OC_LoadThis: // load the current frame's this object
			mStack->emplace_back( frame->thisObject );
			++frame->ip;
			break;

		case OC_StoreLocal: // A is the index in the function scope
			frame->variables[ frame->ip->A ] = mStack->back();
			++frame->ip;
			break;

		case OC_StoreGlobal: // A is the index in the global scope
		{
			unsigned index = unsigned(frame->ip->A);
			if( index >= frame->globals->size() )
				frame->globals->resize(index + 1);
			frame->globals->at(index) = mStack->back();
			++frame->ip;
			break;
		}

		case OC_PopStoreLocal: // A is the index in the function scope
			frame->variables[ frame->ip->A ] = mStack->back();
			mStack->pop_back();
			++frame->ip;
			break;

		case OC_PopStoreGlobal: // A is the index in the global scope
		{
			unsigned index = unsigned(frame->ip->A);
			if( index >= frame->globals->size() )
				frame->globals->resize(index + 1);
			frame->globals->at(index) = mStack->back();
			mStack->pop_back();
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
				array->elements[i] = mStack->back();
				mStack->pop_back();
			}

			mStack->emplace_back(array);

			++frame->ip;
			break;
		}

		case OC_LoadElement: // TOS is the index in the TOS1 array or object
		{
			Value index = mStack->back();
			mStack->pop_back();
			
			Value container = mStack->back();
			mStack->pop_back();
			
			if( container.IsArray() )
			{
				if( ! index.IsInt() )
				{
					SetError("Array index must be an integer");
					return;
				}

				mStack->emplace_back(); // the value to get
				LoadElementFromArray(container.array, index.AsInt(), &mStack->back());
				
				if( HasError() )
					return;
			}
			else if( container.IsObject() )
			{
				if( ! index.IsString() )
				{
					SetError("Object index must be a string");
					return;
				}
				
				mStack->emplace_back(); // the value to get
				LoadMemberFromObject(container.object, GetHashFromName(index.AsString()), &mStack->back());
			}
			else // error
			{
				SetError("The indexing operator only operates on arrays and objects");
				return;
			}
			
			++frame->ip;
			break;
		}

		case OC_StoreElement: // TOS index, TOS1 array or object, TOS2 new value
		{
			Value index = mStack->back();
			mStack->pop_back();
			
			Value container = mStack->back();
			mStack->pop_back();
			
			if( container.IsArray() )
			{
				if( ! index.IsInt() )
				{
					SetError("Array index must be an integer");
					return;
				}

				StoreElementInArray(container.array, index.AsInt(), mStack->back());
				
				if( HasError() )
					return;
			}
			else if( container.IsObject() )
			{
				if( ! index.IsString() )
				{
					SetError("Object index must be a string");
					return;
				}
				
				StoreMemberInObject(container.object, GetHashFromName(index.AsString()), mStack->back());
			}
			else // error
			{
				SetError("The indexing operator only operates on arrays and objects");
				return;
			}
			
			++frame->ip;
			break;
		}

		case OC_PopStoreElement: // TOS index, TOS1 array or object, TOS2 new value
		{
			Value index = mStack->back();
			mStack->pop_back();
			
			Value container = mStack->back();
			mStack->pop_back();
			
			if( container.IsArray() )
			{
				if( ! index.IsInt() )
				{
					SetError("Array index must be an integer");
					return;
				}

				StoreElementInArray(container.array, index.AsInt(), mStack->back());
				
				if( HasError() )
					return;
				
				mStack->pop_back();
			}
			else if( container.IsObject() )
			{
				if( ! index.IsString() )
				{
					SetError("Object index must be a string");
					return;
				}
				
				StoreMemberInObject(container.object, GetHashFromName(index.AsString()), mStack->back());
				mStack->pop_back();
			}
			else // error
			{
				SetError("The indexing operator only operates on arrays and objects");
				return;
			}
			
			++frame->ip;
			break;
		}

		case OC_ArrayPushBack:
		{
			Value newValue = mStack->back();
			mStack->pop_back();

			if( mStack->back().IsArray() )
			{
				PushElementToArray(mStack->back().array, newValue);
				mStack->pop_back();
				mStack->push_back( newValue );
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
			if( mStack->back().IsArray() )
			{
				Array* array = mStack->back().array;
				mStack->pop_back();
				
				Value popped;
				if( PopElementFromArray(array, &popped) )
					mStack->push_back( popped );
				else
					mStack->push_back( mMemoryManager.NewError("empty-array") );
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

				member.value = mStack->back();
				mStack->pop_back();

				member.hash = mStack->back().AsHash();
				mStack->pop_back();
			}

			std::sort(object->members.begin(), object->members.end());

			mStack->emplace_back(object);

			++frame->ip;
			break;
		}

		case OC_MakeEmptyObject: // make an object with just the proto member value
		{
			Object* object = mMemoryManager.NewObject();
			object->members.resize(1);

			object->members[0].hash = Symbol::ProtoHash;

			mStack->emplace_back(object);

			++frame->ip;
			break;
		}

		case OC_LoadHash: // H is the hash to load on the stack
			mStack->emplace_back( frame->ip->H );
			++frame->ip;
			break;

		case OC_LoadMember: // TOS is the member hash in the TOS1 object
		{
			unsigned hash = mStack->back().AsHash();
			mStack->pop_back();

			if( ! mStack->back().IsObject() )
			{
				SetError("Attempt to access a member of a non-object value");
				return;
			}

			mExecutionContext->lastObject = mStack->back();
			mStack->pop_back();
			mStack->emplace_back(); // the value to get
			LoadMemberFromObject(mExecutionContext->lastObject.object, hash, &mStack->back());
			++frame->ip;
			break;
		}

		case OC_StoreMember: // TOS member hash, TOS1 object, TOS2 new value
		{
			unsigned hash = mStack->back().AsHash();
			mStack->pop_back();

			if( ! mStack->back().IsObject() )
			{
				SetError("Attempt to access a member of a non-object value");
				return;
			}

			Object* object = mStack->back().object;
			mStack->pop_back();
			StoreMemberInObject(object, hash, mStack->back());
			++frame->ip;
			break;
		}

		case OC_PopStoreMember: // TOS member hash, TOS1 object, TOS2 new value
		{
			unsigned hash = mStack->back().AsHash();
			mStack->pop_back();

			if( ! mStack->back().IsObject() )
			{
				SetError("Attempt to access a member of a non-object value");
				return;
			}

			Object* object = mStack->back().object;
			mStack->pop_back();
			StoreMemberInObject(object, hash, mStack->back());
			mStack->pop_back();
			++frame->ip;
			break;
		}

		case OC_MakeIterator: // make an iterator object from TOS and replace it at TOS
		{
			Iterator* iterator = MakeIteratorForValue( mStack->back() );
			
			if( iterator )
			{
				mStack->pop_back();
				mStack->emplace_back(iterator);
				++frame->ip;
				break;
			}
			else // error
			{
				if( mStack->back().type == Value::VT_Function && 
					mStack->back().function->executionContext == nullptr )
				{
					SetError("Cannot iterate a function. Only coroutine instances are iterable.");
				}
				else
				{
					SetError("Value not iterable.");
				}
				return;
			}
		}

		case OC_IteratorHasNext: // call 'has_next' from the TOS object
		{
			if( mStack->back().IsIterator() )
			{
				IteratorImplementation* ii = mStack->back().iterator->implementation;
				
				mExecutionContext->lastObject = ii->thisObjectUsed;
				
				mStack->push_back( ii->hasNextFunction );
				
				if( mStack->back().type == Value::VT_NativeFunction )
				{
					CallNative(0);

					if( HasError() )
						return;

					++frame->ip;
				}
				else // normal function
				{
					Call(0);

					++frame->ip;
					return;
				}
			}
			else
			{
				SetError("Value is not an iterator");
				return;
			}
			break;
		}

		case OC_IteratorGetNext: // call 'get_next' from the TOS object
		{
			if( mStack->back().IsIterator() )
			{
				IteratorImplementation* ii = mStack->back().iterator->implementation;
				
				mExecutionContext->lastObject = ii->thisObjectUsed;
				
				mStack->push_back( ii->getNextFunction );
				
				if( mStack->back().type == Value::VT_NativeFunction )
				{
					CallNative(0);

					if( HasError() )
						return;

					++frame->ip;
				}
				else // normal function
				{
					Call(0);

					++frame->ip;
					return;
				}
			}
			else
			{
				SetError("Value is not an iterator");
				return;
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
			mStack->emplace_back( frame->variables[ frame->ip->A ].box->value );
			++frame->ip;
			break;

		case OC_StoreToBox: // A is the index of the box that holds the value
		{
			Box* box = frame->variables[ frame->ip->A ].box;
			Value& newValue = mStack->back();

			box->value = newValue;

			mMemoryManager.UpdateGcRelationship(box, newValue);

			++frame->ip;
			break;
		}

		case OC_PopStoreToBox: // A is the index of the box that holds the value
		{
			Box* box = frame->variables[ frame->ip->A ].box;
			Value& newValue = mStack->back();

			box->value = newValue;

			mMemoryManager.UpdateGcRelationship(box, newValue);

			mStack->pop_back();
			++frame->ip;
			break;
		}

		case OC_MakeClosure: // Create a closure from the function object at TOS and replace it
		{
			Function* newFunction = mMemoryManager.NewFunction( mStack->back().function );

			const std::vector<int>& closureMapping = newFunction->codeObject->closureMapping;

			newFunction->freeVariables.reserve( closureMapping.size() );

			for( int indexToBox : closureMapping )
			{
				if( indexToBox >= 0 )
					newFunction->freeVariables.push_back( frame->variables[ indexToBox ].box );
				else // from a free variable
					newFunction->freeVariables.push_back( frame->function->freeVariables[ -indexToBox - 1 ] );
			}

			mStack->back() = Value(newFunction);

			++frame->ip;
			break;
		}

		case OC_LoadFromClosure: // load the value of the free variable inside the closure at index A
			mStack->emplace_back( frame->function->freeVariables[ frame->ip->A ]->value );
			++frame->ip;
			break;

		case OC_StoreToClosure: // A is the index of the free variable inside the closure
		{
			Box* box = frame->function->freeVariables[ frame->ip->A ];
			Value& newValue = mStack->back();

			box->value = newValue;

			mMemoryManager.UpdateGcRelationship(box, newValue);

			++frame->ip;
			break;
		}

		case OC_PopStoreToClosure: // A is the index of the free variable inside the closure
		{
			Box* box = frame->function->freeVariables[ frame->ip->A ];
			Value& newValue = mStack->back();

			box->value = newValue;

			mMemoryManager.UpdateGcRelationship(box, newValue);

			mStack->pop_back();
			++frame->ip;
			break;
		}

		case OC_Jump: // jump to A
			frame->ip = &frame->instructions[ frame->ip->A ];
			break;

		case OC_JumpIfFalse: // jump to A, if TOS is false
			if( mStack->back().AsBool() )
				++frame->ip;
			else
				frame->ip = &frame->instructions[ frame->ip->A ];
			break;

		case OC_PopJumpIfFalse: // jump to A, if TOS is false, pop TOS either way
			if( mStack->back().AsBool() )
				++frame->ip;
			else
				frame->ip = &frame->instructions[ frame->ip->A ];
			mStack->pop_back();
			break;

		case OC_JumpIfFalseOrPop: // jump to A, if TOS is false, otherwise pop TOS (and-op)
			if( mStack->back().AsBool() )
			{
				mStack->pop_back();
				++frame->ip;
			}
			else
			{
				frame->ip = &frame->instructions[ frame->ip->A ];
			}
			break;

		case OC_JumpIfTrueOrPop: // jump to A, if TOS is true, otherwise pop TOS (or-op)
			if( mStack->back().AsBool() )
			{
				frame->ip = &frame->instructions[ frame->ip->A ];
			}
			else
			{
				mStack->pop_back();
				++frame->ip;
			}
			break;

		case OC_FunctionCall: // function to call and arguments are on stack, A is arguments count
			if( ! mStack->back().IsFunction() )
			{
				SetError("Attempt to call a non-function value");
				return;
			}

			if( mStack->back().type == Value::VT_NativeFunction )
			{
				CallNative( frame->ip->A );

				if( HasError() )
					return;

				++frame->ip;
			}
			else // normal function
			{
				Call( frame->ip->A );

				++frame->ip;
				return;
			}
			break;

		case OC_Yield: // yield the value from TOS to the parent execution context
		{
			if( ! mExecutionContext->parent )
			{
				SetError("Attempt to yield while not in a coroutine");
				return;
			}

			Value yieldValue = mStack->back();
			mStack->pop_back();

			// switch context
			mExecutionContext = mExecutionContext->parent;
			mStack = &mExecutionContext->stack;

            mStack->push_back(yieldValue);

			++frame->ip;
			return;
		}

		case OC_EndFunction: // end function sentinel
			mExecutionContext->stackFrames.pop_back();

			if( mExecutionContext->stackFrames.empty() )
			{
				mExecutionContext->state = ExecutionContext::CRS_Finished;

				if( mExecutionContext->parent )
				{
					Value yieldValue = mStack->back();
					mStack->pop_back();

					// switch context
					mExecutionContext = mExecutionContext->parent;
					mStack = &mExecutionContext->stack;

					mStack->push_back(yieldValue);
				}
			}
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
			if( ! mStack->back().IsNumber() )
			{
				SetError("Unary plus used on a value that is not an integer or float");
				return;
			}

			++frame->ip; // do nothing (:
			break;

		case OC_UnaryMinus:
			if( mStack->back().IsInt() )
			{
				int i = mStack->back().AsInt();
				mStack->pop_back();
				mStack->emplace_back(-i);
			}
			else if( mStack->back().IsFloat() )
			{
				float f = mStack->back().AsFloat();
				mStack->pop_back();
				mStack->emplace_back(-f);
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
			bool b = mStack->back().AsBool(); // anything can be turned into a bool
			mStack->pop_back();
			mStack->emplace_back(!b);
			++frame->ip;
			break;
		}

		case OC_UnaryConcatenate:
		{
			std::string str = mStack->back().AsString(); // anything can be turned into a string
			mStack->pop_back();
			mStack->emplace_back( mMemoryManager.NewString(str) );
			++frame->ip;
			break;
		}

		case OC_UnarySizeOf:
		{
			const Value& value = mStack->back();
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

			mStack->pop_back();
			mStack->emplace_back(size);

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
	Function* function = mStack->back().function;
	mStack->pop_back();

	std::vector<Value>* sourceStack = mStack;

	if( function->executionContext )
	{
		function->executionContext->parent = mExecutionContext;
		
		if( function->executionContext->state == ExecutionContext::CRS_Started )
		{
			Value valueToSend;

			if( argumentsCount == 1 )
			{
				valueToSend = mStack->back();
				mStack->pop_back();
			}
			else if( argumentsCount > 1 ) // we need to pack them into an array
			{
				Array* array = mMemoryManager.NewArray();
				array->elements.resize(argumentsCount);

                std::copy(mStack->end() - argumentsCount, mStack->end(), array->elements.begin());
                mStack->resize(mStack->size() - argumentsCount);

				valueToSend = Value(array);
			}

			// switch context
			mExecutionContext = function->executionContext;
			mStack = &mExecutionContext->stack;

			mStack->push_back(valueToSend);
			return;
		}
		if( function->executionContext->state == ExecutionContext::CRS_Finished )
		{
			mStack->resize(mStack->size() - argumentsCount);
			mStack->push_back( mMemoryManager.NewError("dead-coroutine") );
			return;
		}
		if( function->executionContext->state == ExecutionContext::CRS_NotStarted )
		{
			// we will extract the arguments from the stack of the old context
			sourceStack = mStack;

			// switch context
			mExecutionContext = function->executionContext;
			mExecutionContext->state = ExecutionContext::CRS_Started;
			mStack = &mExecutionContext->stack;
		}
	}

	const CodeObject* codeObject = function->codeObject;

	// create a new stack frame ////////////////////////////////////////////////
	mExecutionContext->stackFrames.emplace_back();
	StackFrame* newFrame = &mExecutionContext->stackFrames.back();

	newFrame->function		= function;
	newFrame->instructions	= codeObject->instructions.data();
	newFrame->ip			= newFrame->instructions;
	newFrame->thisObject	= mExecutionContext->lastObject;
	newFrame->globals		= &codeObject->module->globals;
	
	newFrame->variables.resize( codeObject->localVariablesCount );

	// bind parameters to local variables //////////////////////////////////////
	int anonymousCount = argumentsCount - codeObject->namedParametersCount;

	if( anonymousCount <= 0 )
	{
		std::copy(sourceStack->end() - argumentsCount, sourceStack->end(), newFrame->variables.begin());
	}
	else // we have some anonymous arguments
	{
		newFrame->anonymousParameters.elements.resize( anonymousCount );

		std::copy(sourceStack->end() - argumentsCount, sourceStack->end() - anonymousCount, newFrame->variables.begin());
		std::copy(sourceStack->end() - anonymousCount, sourceStack->end(), newFrame->anonymousParameters.elements.begin());
	}

	sourceStack->resize(sourceStack->size() - argumentsCount);
}

void VirtualMachine::CallNative(int argumentsCount)
{
	Value::NativeFunction function = mStack->back().nativeFunction;
	mStack->pop_back();

	std::vector<Value> arguments;
	arguments.resize(argumentsCount);

	for( int i = argumentsCount - 1; i >= 0; --i )
	{
		arguments[i] = mStack->back();
		mStack->pop_back();
	}
	
	Value result = function(*this, mExecutionContext->lastObject, arguments);

	mStack->push_back(result);
}

void VirtualMachine::PushElementToArray(Array* array, const Value& newValue)
{
	array->elements.push_back(newValue);

	mMemoryManager.UpdateGcRelationship(array, newValue);
}

bool VirtualMachine::PopElementFromArray(Array* array, Value* outValue)
{
	if( !array->elements.empty() )
	{
		*outValue = array->elements.back();
		array->elements.pop_back();
		return true;
	}
	
	return false;
}

void VirtualMachine::LoadElementFromArray(const Array* array, int index, Value* outValue)
{
	int size = int(array->elements.size());

	if( index < 0 )
		index += size;
	
	if( index < 0 || index >= size )
	{
		SetError("Array index out of range");
		return;
	}

	*outValue = array->elements[index];
}

void VirtualMachine::StoreElementInArray(Array* array, int index, const Value& newValue)
{
	int size = int(array->elements.size());

	if( index < 0 )
		index += size;

	if( index < 0 || index >= size )
	{
		SetError("Array index out of range");
		return;
	}

	array->elements[index] = newValue;

	mMemoryManager.UpdateGcRelationship(array, newValue);
}

void VirtualMachine::LoadMemberFromObject(Object* object, unsigned hash, Value* outValue) const
{
	Object::Member member(hash);

	auto it = std::lower_bound(object->members.begin(), object->members.end(), member);

	if( it == object->members.end() || it->hash != hash )
	{
		const Value* proto = &object->members[0].value;

		while(	proto->type == Value::VT_Object && // it has a proto object
				proto->object != object ) // and it is not the first object
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

void VirtualMachine::StoreMemberInObject(Object* object, unsigned hash, const Value& newValue)
{
	Object::Member member(hash);

	auto it = std::lower_bound(object->members.begin(), object->members.end(), member);

	if( it == object->members.end() || it->hash != hash )
	{
		bool found = false;
		const Value* proto = &object->members[0].value;

		while(	proto->type == Value::VT_Object && // it has a proto object
				proto->object != object ) // and it is not the first object
		{
			std::vector<Object::Member>& members = proto->object->members;

			auto it = std::lower_bound(members.begin(), members.end(), member);

			if( it == members.end() || it->hash != hash )
			{
				proto = &members[0].value;
			}
			else // found in one of the proto objects
			{
				it->value = newValue;
				found = true;
				break;
			}
		}

		if( ! found ) // create a new one
			object->members.insert(it, Object::Member(hash, newValue));
	}
	else // found the value corresponding to this hash
	{
		it->value = newValue;
	}

	mMemoryManager.UpdateGcRelationship(object, newValue);
}

bool VirtualMachine::DoBinaryOperation(int opCode)
{
	unsigned last = mStack->size() - 1;
	Value& lhs = mStack->at(last - 1);
	Value& rhs = mStack->at(last);
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
				if( lhs.IsInt() && rhs.IsInt() )
					result = Value( lhs.AsInt() % rhs.AsInt() );
				else
					result = Value( std::fmod(lhs.AsFloat(), rhs.AsFloat()) );
			}
			else
			{
				SetError("Invalid arguments for operator %%");
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
			if( lhs.IsInt() && rhs.IsInt() )
			{
				result = lhs.AsInt() == rhs.AsInt();
			}
			else if( lhs.IsNumber() && rhs.IsNumber() )
			{
				result = lhs.AsFloat() == rhs.AsFloat();
			}
			else if( lhs.type == rhs.type )
			{
				if( lhs.IsNil() )
					result = true;
				else if( lhs.IsBoolean() )
					result = lhs.AsBool() == rhs.AsBool();
				else if( lhs.IsHash() )
					result = lhs.hash == rhs.hash;
				else if( lhs.IsString() || lhs.IsError() )
					result = lhs.AsString() == rhs.AsString();
				else
					result = lhs.object == rhs.object; // compare any pointers
			}
			else
			{
				result = false;
			}
			break;
		}

		case OC_NotEqual:
		{
			if( lhs.IsInt() && rhs.IsInt() )
			{
				result = lhs.AsInt() != rhs.AsInt();
			}
			else if( lhs.IsNumber() && rhs.IsNumber() )
			{
				result = lhs.AsFloat() != rhs.AsFloat();
			}
			else if( lhs.type == rhs.type )
			{
				if( lhs.IsNil() )
					result = false;
				else if( lhs.IsBoolean() )
					result = lhs.AsBool() != rhs.AsBool();
				else if( lhs.IsHash() )
					result = lhs.hash != rhs.hash;
				else if( lhs.IsString() || lhs.IsError() )
					result = lhs.AsString() != rhs.AsString();
				else
					result = lhs.object != rhs.object; // compare any pointers
			}
			else
			{
				result = true;
			}
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

	mStack->pop_back();
	mStack->pop_back();
	mStack->push_back(result);

	return true;
}

void VirtualMachine::RegisterStandardUtilities()
{
	std::string executablePath = mFileManager.GetExecutablePath();
	mFileManager.AddSearchPath(executablePath + "../stdlib");
	
	for( const auto& native : nativefunctions::GetAllFunctions() )
		RegisterNativeFunction(native.name, native.function);
}

void VirtualMachine::LogStackTraceStartingFrom(const StackFrame* frame)
{
	using namespace std::string_literals;
	
	int line = -1;
	std::string oldFilename;
	std::string newFilename;
	LocationFromFrame(frame, &line, &oldFilename);

	mLogger.PushError(line, mErrorMessage);

	mErrorMessage = "called from here";

	if( mExecutionContext )
	{
		mExecutionContext->stackFrames.pop_back();

		ExecutionContext* currentContext = mExecutionContext;

		while( currentContext )
		{
			std::deque<StackFrame>& stackFrames = currentContext->stackFrames;

			while( ! stackFrames.empty() )
			{
				LocationFromFrame(&stackFrames.back(), &line, &newFilename);
				
				if( oldFilename != newFilename )
				{
					mLogger.PushError("file: "s + oldFilename);
					oldFilename = newFilename;
				}
				
				mLogger.PushError(line, mErrorMessage);

				stackFrames.pop_back();
			}

			currentContext = currentContext->parent;
		}
	}
}

void VirtualMachine::LocationFromFrame(const StackFrame* frame, int* currentLine, std::string* currentFile) const
{
	const CodeObject* codeObject = frame->function->codeObject;
	
	*currentLine = -1;
	*currentFile = codeObject->module->filename;
	
	const auto& lines = codeObject->instructionLines;

	if( lines.empty() )
		return;

	if( lines.size() == 1 )
	{
		*currentLine = lines.back().line;
		return;
	}
	
	int instructionIndex = frame->ip - codeObject->instructions.data();

	int lineIndex = -1;

	for( const SourceCodeLine& line : lines )
	{
		if( instructionIndex >= line.instructionIndex )
		{
			lineIndex = line.line;
		}
		else
		{
			*currentLine = lineIndex;
			return;
		}
	}

	*currentLine = lineIndex;
}

}
