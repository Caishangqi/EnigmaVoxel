// ChunkWorker.cpp
#include "ChunkWorker.h"
#include "ChunkWorkerPool.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"

FChunkWorker::FChunkWorker(UChunkWorkerPool& InPool, int InId)
	: Pool(InPool), Id(InId)
{
	WakeEvent = FPlatformProcess::GetSynchEventFromPool(false);
	UE_LOG(LogEnigmaVoxelWorker, Log, TEXT("Worker[%d] spawned"), Id);
}

FChunkWorker::~FChunkWorker()
{
	FPlatformProcess::ReturnSynchEventToPool(WakeEvent);
	UE_LOG(LogEnigmaVoxelWorker, Log, TEXT("Worker[%d] destroyed"), Id);
}

void FChunkWorker::Stop()
{
	bStop = true;
	WakeEvent->Trigger();
}

void FChunkWorker::Wake()
{
	WakeEvent->Trigger();
}

uint32 FChunkWorker::Run()
{
	while (!bStop)
	{
		TUniqueFunction<void()> Job;
		if (Pool.DequeueJob(Job))
		{
			bIdle = false;
			Job();
			bIdle = true;
		}
		else
		{
			WakeEvent->Wait(5);
		}
	}
	return 0;
}
