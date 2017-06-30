#ifndef _MEMORY_MANAGER_INCLUDED_
#define _MEMORY_MANAGER_INCLUDED_

#include <deque>
#include <limits>

#include "DataTypes.h"

namespace element
{

class MemoryManager
{
public:			MemoryManager();
				~MemoryManager();
	
	String*		NewString();
	String*		NewString(const std::string& str);
	String*		NewString(const char* str, int size);
	Array*		NewArray();
	Object*		NewObject();
	Object*		NewObject(const Object* other);
	Function*	NewFunction(const Function* other);
	Box*		NewBox();
	Box*		NewBox(const Value& value);
	Generator*	NewGeneratorArray(Array* array);
	Generator*	NewGeneratorString(String* str);
	Generator*	NewGeneratorNative(GeneratorImplementation* newGenerator);

	int			GarbageCollectionStepsToSchedule();
	
	void		GarbageCollect(int steps = std::numeric_limits<int>::max());
	
	void		EnableGarbageCollection(bool enable);
	bool		IsGarbageCollectionEnabled() const;
	bool		IsGarbageCollecting() const;
	
	void		ClearGrayList();
	void		MakeGray(GarbageCollected* gc);
	
	unsigned	GetHeapObjectsCount(Value::Type type) const;

	GarbageCollected::State GetCurrentWhite() const;

protected:
	enum GCStage : char
	{
		GCS_Ready		= 0,
		GCS_Mark		= 1,
		GCS_SweepHead	= 2,
		GCS_SweepRest	= 3,
	};

protected:
	void		DeleteHeap();
	void		FreeGC(GarbageCollected* gc);
	
	int			Mark(int steps);
	int			SweepHead(int steps);
	int			SweepRest(int steps);
	
private:
	GarbageCollected*				mHeapHead;
	
	bool							mGarbageCollectionEnabled;
	GCStage							mGCStage;
	GarbageCollected::State			mCurrentWhite;
	GarbageCollected::State			mNextWhite;
	std::deque<GarbageCollected*>	mGrayList;
	GarbageCollected*				mPreviousGC;
	GarbageCollected*				mCurrentGC;

	unsigned						mHeapStringsCount;
	unsigned						mHeapArraysCount;
	unsigned						mHeapObjectsCount;
	unsigned						mHeapFunctionsCount;
	unsigned						mHeapBoxesCount;
	unsigned						mHeapGeneratorsCount;
};

}

#endif // _MEMORY_MANAGER_INCLUDED_
