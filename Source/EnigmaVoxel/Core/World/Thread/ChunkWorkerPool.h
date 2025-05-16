#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SharedPointer.h"
#include "ChunkWorkerPool.generated.h"

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
	bool EnqueueBuildTask(FChunkHolder* Holder, bool bMeshOnly); // 外部调用
	bool DequeueJob(TUniqueFunction<void()>& Out); // 被 Worker 调

private:
	/// Internal Structure
	struct FQueued
	{
		FIntVector                 Key;
		TSharedPtr<TPromise<void>> Promise;
		TFunction<void()>          Func;
	};

	/* === 数据 === */
	TQueue<FQueued*>           Pending;
	FCriticalSection           Mutex;
	TMap<FIntVector, FQueued*> Running; // 去重
	TArray<FChunkWorker*>      Workers;
	TArray<FRunnableThread*>   Threads;
	FThreadSafeBool            bStopping{false};

	// Helper function
	void WakeAnyIdleWorker();
};
