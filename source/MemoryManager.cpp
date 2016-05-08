#include "MemoryManager.h"


namespace element
{

MemoryManager::MemoryManager()
: mHeapHead(nullptr)
, mGarbageCollectionEnabled(true)
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
, mHeapGeneratorsCount(0)
{
}

MemoryManager::~MemoryManager()
{
	DeleteHeap();
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

Generator* MemoryManager::NewGeneratorArray(Array* array)
{
	Generator* newGenerator = new Generator(new ArrayGenerator(array));

	newGenerator->state = mNextWhite;

	if( mHeapHead )
		newGenerator->next = mHeapHead;
	mHeapHead = newGenerator;

	++mHeapGeneratorsCount;

	return newGenerator;
}

Generator* MemoryManager::NewGeneratorString(String* str)
{
	Generator* newGenerator = new Generator(new StringGenerator(str));

	newGenerator->state = mNextWhite;

	if( mHeapHead )
		newGenerator->next = mHeapHead;
	mHeapHead = newGenerator;

	++mHeapGeneratorsCount;

	return newGenerator;
}

Generator* MemoryManager::NewGeneratorNative(GeneratorImplementation* newGenerator)
{
	Generator* generator = new Generator(newGenerator);

	generator->state = mNextWhite;

	if( mHeapHead )
		generator->next = mHeapHead;
	mHeapHead = generator;

	++mHeapGeneratorsCount;

	return generator;
}

int MemoryManager::GarbageCollectionStepsToSchedule()
{
	if( ! mGarbageCollectionEnabled )
		return 0;
	
	// some magic code to determine how badly do we need to garbage collect...
	return 100;
}

void MemoryManager::GarbageCollect(int steps)
{
	switch( mGCStage )
	{
	case GCS_Ready:
		std::swap(mCurrentWhite, mNextWhite); // White0 <-> White1
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

void MemoryManager::EnableGarbageCollection(bool enable)
{
	mGarbageCollectionEnabled = enable;
}

bool MemoryManager::IsGarbageCollectionEnabled() const
{
	return mGarbageCollectionEnabled;
}

bool MemoryManager::IsGarbageCollecting() const
{
	return mGCStage != GCS_Ready;
}

void MemoryManager::ClearGrayList()
{
	mGrayList.clear();
}

void MemoryManager::MakeGray(GarbageCollected* gc)
{
	gc->state = GarbageCollected::State::GC_Gray;
	mGrayList.push_back(gc);
}

unsigned MemoryManager::GetHeapObjectsCount(Value::Type type) const
{
	switch( type )
	{
	case Value::VT_String:			return mHeapStringsCount;
	case Value::VT_Array:			return mHeapArraysCount;
	case Value::VT_Object:			return mHeapObjectsCount;
	case Value::VT_Function:		return mHeapFunctionsCount;
	case Value::VT_Box:				return mHeapBoxesCount;
	case Value::VT_NativeGenerator:	return mHeapGeneratorsCount;
	default:						return 0;
	}
}

GarbageCollected::State MemoryManager::GetCurrentWhite() const
{
	return mCurrentWhite;
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
		delete (Function*)gc;
		--mHeapFunctionsCount;
		break;

	case Value::VT_Box:
		delete (Box*)gc;
		--mHeapBoxesCount;
		break;

	case Value::VT_NativeGenerator:
		delete (Generator*)gc; // virtual call
		--mHeapGeneratorsCount;
		break;

	default:
		break;
	}
}

int MemoryManager::Mark(int steps)
{
	GarbageCollected* gc = nullptr;
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
			{
				if( element.IsGarbageCollected() )
				{
					gc = element.garbageCollected;
					if( gc->state == mCurrentWhite )
					{
						gc->state = GarbageCollected::GC_Gray;
						mGrayList.push_back(gc);
						
						steps -= 1;
					}
				}
			}
			break;

		case Value::VT_Object:
			for( Object::Member& member : ((Object*)currentObject)->members )
			{
				if( member.value.IsGarbageCollected() )
				{
					gc = member.value.garbageCollected;
					if( gc->state == mCurrentWhite )
					{
						gc->state = GarbageCollected::GC_Gray;
						mGrayList.push_back(gc);
						
						steps -= 1;
					}
				}
			}
			break;

		case Value::VT_Function:
			for( Box* box : ((Function*)currentObject)->boxes )
			{
				if( box && box->state == mCurrentWhite )
				{
					box->state = GarbageCollected::GC_Gray;
					mGrayList.push_back(box);
				}
			}
			break;

		case Value::VT_Box:
		{
			Value& value = ((Box*)currentObject)->value;
			if( value.IsGarbageCollected() )
			{
				gc = value.garbageCollected;
				if( gc->state == mCurrentWhite )
				{
					gc->state = GarbageCollected::GC_Gray;
					mGrayList.push_back(gc);
				}
			}
			break;
		}

		case Value::VT_NativeGenerator:
			((Generator*)currentObject)->implementation->UpdateGrayList(mGrayList, mCurrentWhite); // virtual call
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
