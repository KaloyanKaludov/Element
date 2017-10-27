#ifndef _MEMORY_MANAGER_INCLUDED_
#define _MEMORY_MANAGER_INCLUDED_

#include <deque>
#include <limits>
#include <memory>
#include <unordered_map>

#include "DataTypes.h"
#include "GarbageCollected.h"

namespace element
{

class MemoryManager
{
public:					MemoryManager();
						~MemoryManager();
						
	void				ResetState();
	
	Module&				GetDefaultModule();
	Module&				GetModuleForFile(const std::string& filename);

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
	Iterator*			NewIterator(IteratorImplementation* newIterator);
	Error*				NewError(const std::string& errorMessage);

	ExecutionContext*	NewRootExecutionContext();
	bool				DeleteRootExecutionContext(ExecutionContext* context);

	void				GarbageCollect(int steps = std::numeric_limits<int>::max());

	void				UpdateGcRelationship(GarbageCollected* parent, const Value& child);

	int					GetHeapObjectsCount(Value::Type type) const;
	
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
	void		AddToHeap(GarbageCollected* gc);
	void		FreeGC(GarbageCollected* gc);
	void		MakeGrayIfNeeded(GarbageCollected* gc, int* steps);

	int			MarkRoots(int steps);
	int			Mark(int steps);
	int			SweepHead(int steps);
	int			SweepRest(int steps);

private:
	GarbageCollected*						mHeapHead;

	GCStage									mGCStage;
	GarbageCollected::State					mCurrentWhite;
	GarbageCollected::State					mNextWhite;
	std::deque<GarbageCollected*>			mGrayList;
	GarbageCollected*						mPreviousGC;
	GarbageCollected*						mCurrentGC;

	// memory roots
	Module									mDefaultModule;
	std::unordered_map<std::string, Module>	mModules;
	std::vector<ExecutionContext*>			mExecutionContexts;
	
	// statistics
	int										mHeapStringsCount;
	int										mHeapArraysCount;
	int										mHeapObjectsCount;
	int										mHeapFunctionsCount;
	int										mHeapBoxesCount;
	int										mHeapIteratorsCount;
	int										mHeapErrorsCount;
};

}

#endif // _MEMORY_MANAGER_INCLUDED_
