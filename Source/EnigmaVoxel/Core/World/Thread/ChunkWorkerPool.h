// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ChunkWorkerPool.generated.h"

class FChunkWorker;
/**
 * 
 */
UCLASS()
class ENIGMAVOXEL_API UChunkWorkerPool : public UObject
{
	GENERATED_BODY()

public:
	UChunkWorkerPool();

	bool Init(int32 NumThreads);
	void RequestExit();
	void Shutdown();

	// Returns an idle Worker; if there is no idle Worker, returns nullptr
	FChunkWorker* GetIdleWorker();

	/** Statistics interface */
	int32 GetNumThreads() const { return Workers.Num(); }
	int32 GetNumIdleThreads() const { return IdleCounter.GetValue(); }

private:
	TArray<FChunkWorker*>    Workers; // One-to-one correspondence with Threads index
	TArray<FRunnableThread*> Threads;

	TAtomic<bool> bStopping{false};

	FThreadSafeCounter IdleCounter;
};
