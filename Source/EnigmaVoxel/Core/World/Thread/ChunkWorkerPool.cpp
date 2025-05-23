﻿#include "ChunkWorkerPool.h"
#include "ChunkWorker.h"
#include "EnigmaVoxel/Core/World/Gen/WorldGen.hpp"
#include "EnigmaVoxel/Modules/Chunk/ChunkHolder.h"

bool UChunkWorkerPool::Init(int32 ThreadNum)
{
	if (ThreadNum <= 0)
	{
		ThreadNum = FMath::Max(1, FPlatformMisc::NumberOfCores());
	}
	for (int i = 0; i < ThreadNum; ++i)
	{
		FChunkWorker*    W = new FChunkWorker(*this, i);
		FRunnableThread* T = FRunnableThread::Create(W, *FString::Printf(TEXT("ChunkWorker-%d"), i));
		if (!T)
		{
			return false;
		}
		Workers.Add(W);
		Threads.Add(T);
	}
	return true;
}

void UChunkWorkerPool::Shutdown()
{
	bStopping = true;
	for (FChunkWorker* W : Workers)
	{
		W->Stop();
	}
	for (FRunnableThread* T : Threads)
	{
		T->WaitForCompletion();
		delete T;
	}
	Workers.Empty();
	Threads.Empty();
}

// Task Release
bool UChunkWorkerPool::EnqueueBuildTask(FChunkHolder* Holder, bool bMeshOnly, UEnigmaWorld* World)
{
	const FIntVector Key = Holder->Coords;

	FQueued* NewJob = nullptr;
	{
		FScopeLock _(&Mutex);
		if (Running.Contains(Key))
		{
			return false; // There is already a task with the same coordinates
		}

		TSharedPtr<TPromise<void>> Promise = MakeShared<TPromise<void>>();
		Holder->BuildFuture                = MakeShared<TFuture<void>>(Promise->GetFuture());

		NewJob          = new FQueued;
		NewJob->Key     = Key;
		NewJob->Promise = MakeShared<TPromise<void>>();
		Holder->bNeedsNeighborNotify.store(!bMeshOnly, std::memory_order_relaxed);
		NewJob->Func = [Promise,Holder,bMeshOnly,World]()
		{
			if (bMeshOnly)
			{
				FWorldGen::RebuildMesh(World, *Holder);
			}
			else
			{
				FWorldGen::GenerateFullChunk(World, *Holder);
			}
			Promise->SetValue();
			Holder->Stage = EChunkStage::Ready;
		};
		Running.Add(Key, NewJob);
		Pending.Enqueue(NewJob);
	}
	WakeAnyIdleWorker();
	return true;
}

// Called By worker
bool UChunkWorkerPool::DequeueJob(TUniqueFunction<void()>& Out)
{
	FQueued* J = nullptr;
	{
		FScopeLock _(&Mutex);
		if (!Pending.Dequeue(J))
		{
			return false;
		}
		Running.Remove(J->Key);
	}

	// Let FQueued end with the Job lifecycle
	TUniquePtr<FQueued> Task(J);
	Out = [Task = MoveTemp(Task)]() mutable
	{
		Task->Func(); // Real work
		if (Task->Promise.IsValid())
		{
			Task->Promise->SetValue();
		}
	};
	return true;
}

void UChunkWorkerPool::WakeAnyIdleWorker()
{
	for (FChunkWorker* W : Workers)
	{
		if (W->IsIdle())
		{
			W->Wake();
			break;
		}
	}
}
