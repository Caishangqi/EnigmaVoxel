#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SharedPointer.h"
#include "ChunkWorkerPool.generated.h"

class UEnigmaWorld;
class FChunkWorker;
struct FChunkHolder;

UCLASS()
class ENIGMAVOXEL_API UChunkWorkerPool : public UObject
{
	GENERATED_BODY()

public:
	bool Init(int32 ThreadNum);
	void Shutdown();

	// Task interface
	bool EnqueueBuildTask(FChunkHolder* Holder, bool bMeshOnly, UEnigmaWorld* World = nullptr); // Called by external
	bool DequeueJob(TUniqueFunction<void()>& Out); // Called by worker

private:
	/// Internal Structure that hold lambda and future promise
	struct FQueued
	{
		FIntVector                 Key;
		TSharedPtr<TPromise<void>> Promise;
		TFunction<void()>          Func;
	};

	// Data
	TQueue<FQueued*>           Pending;
	FCriticalSection           Mutex;
	TMap<FIntVector, FQueued*> Running; // Remove duplicates
	TArray<FChunkWorker*>      Workers;
	TArray<FRunnableThread*>   Threads;
	FThreadSafeBool            bStopping{false};

	// Helper function
	void WakeAnyIdleWorker();
};
