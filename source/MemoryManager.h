#ifndef _MEMORY_MANAGER_INCLUDED_
#define _MEMORY_MANAGER_INCLUDED_

#include <deque>
#include <limits>

#include "DataTypes.h"
#include "GarbageCollected.h"

namespace element
{

class MemoryManager
{
public:					MemoryManager();
						~MemoryManager();

	std::vector<Value>&	GetTheGlobals();

	String*				NewString();
	String*				NewString(const std::string& str);
	String*				NewString(const char* str, int size);
	Array*				NewArray();
	Object*				NewObject();
	Object*				NewObject(const Object* other);
	Function*			NewFunction(const Function* other);
	Function*			NewCoroutine(const Function* other);
	Box*				NewBox();
	Box*				NewBox(const Value& value);
	Generator*			NewGeneratorArray(Array* array);
	Generator*			NewGeneratorString(String* str);
	Generator*			NewGeneratorNative(GeneratorImplementation* newGenerator);
	Error*				NewError(const std::string& errorMessage);

	ExecutionContext*	NewRootExecutionContext();
	bool				DeleteRootExecutionContext(ExecutionContext* context);

	void				GarbageCollect(int steps = std::numeric_limits<int>::max());

	void				UpdateGcRelationship(GarbageCollected* parent, Value& child);

	unsigned			GetHeapObjectsCount(Value::Type type) const;

protected:
	enum GCStage : char
	{
		GCS_Ready		= 0,
		GCS_MarkRoots	= 1,
		GCS_Mark		= 2,
		GCS_SweepHead	= 3,
		GCS_SweepRest	= 4,
	};

protected:
	void		DeleteHeap();
	void		FreeGC(GarbageCollected* gc);
	void		MakeGrayIfNeeded(GarbageCollected* gc, int* steps);

	int			MarkRoots(int steps);
	int			Mark(int steps);
	int			SweepHead(int steps);
	int			SweepRest(int steps);

private:
	GarbageCollected*				mHeapHead;

	GCStage							mGCStage;
	GarbageCollected::State			mCurrentWhite;
	GarbageCollected::State			mNextWhite;
	std::deque<GarbageCollected*>	mGrayList;
	GarbageCollected*				mPreviousGC;
	GarbageCollected*				mCurrentGC;

	// memory roots
	std::vector<Value>				mGlobals;
	std::vector<ExecutionContext*>	mExecutionContexts;

	// statistics
	unsigned						mHeapStringsCount;
	unsigned						mHeapArraysCount;
	unsigned						mHeapObjectsCount;
	unsigned						mHeapFunctionsCount;
	unsigned						mHeapBoxesCount;
	unsigned						mHeapGeneratorsCount;
	unsigned						mHeapErrorsCount;
};

}

#endif // _MEMORY_MANAGER_INCLUDED_
