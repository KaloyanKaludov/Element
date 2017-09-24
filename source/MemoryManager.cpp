#include "MemoryManager.h"

#include <algorithm>

namespace element
{

MemoryManager::MemoryManager()
: mHeapHead(nullptr)
, mGCStage(GCS_Ready)
, mCurrentWhite(GarbageCollected::GC_White0)
, mNextWhite(GarbageCollected::GC_White1)
, mPreviousGC(nullptr)
, mCurrentGC(nullptr)
, mHeapStringsCount(0)
, mHeapArraysCount(0)
, mHeapObjectsCount(0)
, mHeapFunctionsCount(0)
, mHeapBoxesCount(0)
, mHeapIteratorsCount(0)
, mHeapErrorsCount(0)
{
}

MemoryManager::~MemoryManager()
{
	DeleteHeap();
}

void MemoryManager::ResetState()
{
	mGlobals.clear();
	mExecutionContexts.clear();
	
	DeleteHeap();
	
	mGCStage		= GCS_Ready;
	mCurrentWhite	= GarbageCollected::GC_White0;
	mNextWhite		= GarbageCollected::GC_White1;
	mPreviousGC		= nullptr;
	mCurrentGC		= nullptr;
	
	mHeapStringsCount	= 0;
	mHeapArraysCount	= 0;
	mHeapObjectsCount	= 0;
	mHeapFunctionsCount	= 0;
	mHeapBoxesCount		= 0;
	mHeapIteratorsCount = 0;
	mHeapErrorsCount	= 0;
}

std::vector<Value>& MemoryManager::GetTheGlobals()
{
	return mGlobals;
}

String* MemoryManager::NewString()
{
	String* newString = new String();

	newString->state = mNextWhite;

	if( mHeapHead )
		newString->next = mHeapHead;
	mHeapHead = newString;

	++mHeapStringsCount;

	return newString;
}

String*	MemoryManager::NewString(const std::string& str)
{
	String* newString = new String(str);

	newString->state = mNextWhite;

	if( mHeapHead )
		newString->next = mHeapHead;
	mHeapHead = newString;

	++mHeapStringsCount;

	return newString;
}

String*	MemoryManager::NewString(const char* str, int size)
{
	String* newString = new String(str, size);

	newString->state = mNextWhite;

	if( mHeapHead )
		newString->next = mHeapHead;
	mHeapHead = newString;

	++mHeapStringsCount;

	return newString;
}

Array* MemoryManager::NewArray()
{
	Array* newArray = new Array();

	newArray->state = mNextWhite;

	if( mHeapHead )
		newArray->next = mHeapHead;
	mHeapHead = newArray;

	++mHeapArraysCount;

	return newArray;
}

Object* MemoryManager::NewObject()
{
	Object* newObject = new Object();

	newObject->state = mNextWhite;

	if( mHeapHead )
		newObject->next = mHeapHead;
	mHeapHead = newObject;

	++mHeapObjectsCount;

	return newObject;
}

Object* MemoryManager::NewObject(const Object* other)
{
	Object* newObject = new Object();
	*newObject = *other;

	newObject->state = mNextWhite;

	if( mHeapHead )
		newObject->next = mHeapHead;
	mHeapHead = newObject;

	++mHeapObjectsCount;

	return newObject;
}

Function* MemoryManager::NewFunction(const Function* other)
{
	Function* newFunction = new Function(other);

	newFunction->state = mNextWhite;

	if( mHeapHead )
		newFunction->next = mHeapHead;
	mHeapHead = newFunction;

	++mHeapFunctionsCount;

	return newFunction;
}

Function* MemoryManager::NewCoroutine(const Function* other)
{
	Function* newFunction = NewFunction(other);

	newFunction->executionContext = new ExecutionContext();

	return newFunction;
}

Box* MemoryManager::NewBox()
{
	Box* newBox = new Box();

	newBox->state = mNextWhite;

	if( mHeapHead )
		newBox->next = mHeapHead;
	mHeapHead = newBox;

	++mHeapBoxesCount;

	return newBox;
}

Box* MemoryManager::NewBox(const Value& value)
{
	Box* newBox = new Box();

	newBox->value = value;
	newBox->state = mNextWhite;

	if( mHeapHead )
		newBox->next = mHeapHead;
	mHeapHead = newBox;

	++mHeapBoxesCount;

	return newBox;
}

Iterator* MemoryManager::NewIterator(IteratorImplementation* newIterator)
{
	Iterator* iterator = new Iterator(newIterator);

	iterator->state = mNextWhite;

	if( mHeapHead )
		iterator->next = mHeapHead;
	mHeapHead = iterator;

	++mHeapIteratorsCount;

	return iterator;
}

Error* MemoryManager::NewError(const std::string& errorMessage)
{
	Error* newError = new Error(errorMessage);

	newError->state = mNextWhite;

	if( mHeapHead )
		newError->next = mHeapHead;
	mHeapHead = newError;

	++mHeapErrorsCount;

	return newError;
}

ExecutionContext* MemoryManager::NewRootExecutionContext()
{
    ExecutionContext* newContext = new ExecutionContext();

    mExecutionContexts.push_back(newContext);

    return newContext;
}

bool MemoryManager::DeleteRootExecutionContext(ExecutionContext* context)
{
	auto it = std::find(mExecutionContexts.begin(), mExecutionContexts.end(), context);

	if( it != mExecutionContexts.end() )
	{
		mExecutionContexts.erase(it);
		delete context;
		return true;
	}

	return false;
}

void MemoryManager::GarbageCollect(int steps)
{
	switch( mGCStage )
	{
	case GCS_Ready:
		mGrayList.clear();
		std::swap(mCurrentWhite, mNextWhite); // White0 <-> White1
		mGCStage = GCS_MarkRoots;

	case GCS_MarkRoots:
		steps = MarkRoots(steps);
		if( steps <= 0 )
			return;
		mGCStage = GCS_Mark;

	case GCS_Mark:
		steps = Mark(steps);
		if( steps <= 0 )
			return;
		mGCStage = GCS_SweepHead;

	case GCS_SweepHead:
		steps = SweepHead(steps);
		if( steps <= 0 )
			return;
		mGCStage = GCS_SweepRest;

	case GCS_SweepRest:
		steps = SweepRest(steps);
		if( steps <= 0 )
			return;
		mGCStage = GCS_Ready;
	}
}

void MemoryManager::UpdateGcRelationship(GarbageCollected* parent, const Value& child)
{
	// the tri-color invariant states that at no point shall
	// a black node be directly connected to a white node
	if( parent->state == GarbageCollected::GC_Black &&
		child.IsGarbageCollected() &&
		child.garbageCollected->state == mCurrentWhite )
	{
		child.garbageCollected->state = GarbageCollected::State::GC_Gray;
		mGrayList.push_back( child.garbageCollected );
	}
}

int MemoryManager::GetHeapObjectsCount(Value::Type type) const
{
	switch( type )
	{
	case Value::VT_String:		return mHeapStringsCount;
	case Value::VT_Array:		return mHeapArraysCount;
	case Value::VT_Object:		return mHeapObjectsCount;
	case Value::VT_Function:	return mHeapFunctionsCount;
	case Value::VT_Box:			return mHeapBoxesCount;
	case Value::VT_Iterator:	return mHeapIteratorsCount;
	case Value::VT_Error:		return mHeapErrorsCount;
	default:					return 0;
	}
}

void MemoryManager::DeleteHeap()
{
	while( mHeapHead )
	{
		GarbageCollected* next = mHeapHead->next;;
		FreeGC(mHeapHead);
		mHeapHead = next;
	}
}

void MemoryManager::FreeGC(GarbageCollected* gc)
{
	switch( gc->type )
	{
	case Value::VT_String:
		delete (String*)gc;
		--mHeapStringsCount;
		break;

	case Value::VT_Array:
		delete (Array*)gc;
		--mHeapArraysCount;
		break;

	case Value::VT_Object:
		delete (Object*)gc;
		--mHeapObjectsCount;
		break;

	case Value::VT_Function:
	{
		Function* f = (Function*)gc;
		if( f->executionContext )
			delete f->executionContext;
		delete f;
		--mHeapFunctionsCount;
		break;
	}
	case Value::VT_Box:
		delete (Box*)gc;
		--mHeapBoxesCount;
		break;

	case Value::VT_Iterator:
		delete (Iterator*)gc; // virtual call
		--mHeapIteratorsCount;
		break;

	case Value::VT_Error:
		delete (Error*)gc;
		--mHeapErrorsCount;
		break;

	default:
		break;
	}
}

void MemoryManager::MakeGrayIfNeeded(GarbageCollected* gc, int* steps)
{
	if( gc->state == mCurrentWhite )
	{
		gc->state = GarbageCollected::GC_Gray;
		mGrayList.push_back(gc);

		*steps -= 1;
	}
}

int MemoryManager::MarkRoots(int steps)
{
	for( Value& global : mGlobals )
		if( global.IsGarbageCollected() )
			MakeGrayIfNeeded(global.garbageCollected, &steps);

	for( ExecutionContext* context : mExecutionContexts )
	{
		for( StackFrame& frame : context->stackFrames )
		{
			for( Value& local : frame.variables )
				if( local.IsGarbageCollected() )
					MakeGrayIfNeeded(local.garbageCollected, &steps);

			for( Value& anonymousParameter : frame.anonymousParameters.elements )
				if( anonymousParameter.IsGarbageCollected() )
					MakeGrayIfNeeded(anonymousParameter.garbageCollected, &steps);
		}

		for( Value& value : context->stack )
			if( value.IsGarbageCollected() )
				MakeGrayIfNeeded(value.garbageCollected, &steps);
	}

	return steps;
}

int MemoryManager::Mark(int steps)
{
	GarbageCollected* currentObject = nullptr;

	while( ! mGrayList.empty() && steps > 0 )
	{
		currentObject = mGrayList.back();
		currentObject->state = GarbageCollected::GC_Black;
		mGrayList.pop_back();
		steps -= 1;

		switch( currentObject->type )
		{
		case Value::VT_Array:
			for( Value& element : ((Array*)currentObject)->elements )
				if( element.IsGarbageCollected() )
					MakeGrayIfNeeded(element.garbageCollected, &steps);
			break;

		case Value::VT_Object:
			for( Object::Member& member : ((Object*)currentObject)->members )
				if( member.value.IsGarbageCollected() )
					MakeGrayIfNeeded(member.value.garbageCollected, &steps);
			break;

		case Value::VT_Function:
		{
			Function* function = ((Function*)currentObject);
			for( Box* box : function->freeVariables )
				if( box )
					MakeGrayIfNeeded(box, &steps);

			if( function->executionContext )
			{
				for( StackFrame& frame : function->executionContext->stackFrames )
				{
					for( Value& local : frame.variables )
						if( local.IsGarbageCollected() )
							MakeGrayIfNeeded(local.garbageCollected, &steps);

					for( Value& anonymousParameter : frame.anonymousParameters.elements )
						if( anonymousParameter.IsGarbageCollected() )
							MakeGrayIfNeeded(anonymousParameter.garbageCollected, &steps);
				}

				for( Value& value : function->executionContext->stack )
					if( value.IsGarbageCollected() )
						MakeGrayIfNeeded(value.garbageCollected, &steps);
			}
			break;
		}

		case Value::VT_Box:
		{
			Value& value = ((Box*)currentObject)->value;
			if( value.IsGarbageCollected() )
				MakeGrayIfNeeded(value.garbageCollected, &steps);
			break;
		}

		case Value::VT_Iterator:
			((Iterator*)currentObject)->implementation->UpdateGrayList(mGrayList, mCurrentWhite); // virtual call
			break;

		default:
			break;
		}
	}

	return steps;
}

int MemoryManager::SweepHead(int steps)
{
	while( mHeapHead && steps > 0 )
	{
		if( mHeapHead->state == mCurrentWhite )
		{
			GarbageCollected* next = mHeapHead->next;
			FreeGC(mHeapHead);
			mHeapHead = next;
		}
		else
		{
			mHeapHead->state = mNextWhite;
			mPreviousGC = mHeapHead;
			mCurrentGC = mHeapHead->next;
			break;
		}

		steps -= 1;
	}

	return steps;
}

int MemoryManager::SweepRest(int steps)
{
	while( mCurrentGC && steps > 0 )
	{
		if( mCurrentGC->state == mCurrentWhite )
		{
			mPreviousGC->next = mCurrentGC->next;
			FreeGC(mCurrentGC);
			mCurrentGC = mPreviousGC->next;
		}
		else
		{
			mCurrentGC->state = mNextWhite;
			mPreviousGC = mCurrentGC;
			mCurrentGC = mCurrentGC->next;
		}

		steps -= 1;
	}

	return steps;
}

}
