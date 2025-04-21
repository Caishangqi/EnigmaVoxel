// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkWorkerPool.h"

#include "ChunkWorker.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"

UChunkWorkerPool::UChunkWorkerPool()
{
}

bool UChunkWorkerPool::Init(int32 NumThreads)
{
	UE_LOG(LogEnigmaVoxelChunk, Warning, TEXT("UChunkWorkerPool::Init Preparing Create UChunkWorkerPool with %d workers"), NumThreads);
	if (NumThreads <= 0)
	{
		NumThreads = FMath::Max(1, FPlatformMisc::NumberOfCores());
	}

	for (int32 i = 0; i < NumThreads; ++i)
	{
		FString          ThreadName = FString(TEXT("Chunk Worker Thread: %d"), i);
		auto*            Worker     = new FChunkWorker(IdleCounter, i);
		FRunnableThread* Thread     = FRunnableThread::Create(Worker, *ThreadName, 0, TPri_Normal);
		if (!Thread)
		{
			return false;
		}

		Workers.Add(Worker);
		Threads.Add(Thread);
	}
	UE_LOG(LogEnigmaVoxelChunk, Warning, TEXT("UChunkWorkerPool::Init Created with %d threads and %d workers"), NumThreads, Workers.Num());
	return true;
}

void UChunkWorkerPool::RequestExit()
{
	bStopping = true;
	for (FChunkWorker* W : Workers)
	{
		if (W)
		{
			W->Stop();
		}
	}
}

void UChunkWorkerPool::Shutdown()
{
	for (int32 i = 0; i < Threads.Num(); ++i)
	{
		if (Threads[i])
		{
			Threads[i]->WaitForCompletion(); // Halts the caller
			delete Threads[i];
		}
		delete Workers[i];
	}
	Threads.Empty();
	Workers.Empty();
	UE_LOG(LogEnigmaVoxelChunk, Warning, TEXT("UChunkWorkerPool::Shutdown Shutting down and release workers."))
}

FChunkWorker* UChunkWorkerPool::GetIdleWorker()
{
	for (FChunkWorker* W : Workers)
	{
		if (W && W->IsIdle())
		{
			return W;
		}
	}
	return nullptr;
}
