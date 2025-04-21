#include "ChunkWorker.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"

FChunkWorker::FChunkWorker(FThreadSafeCounter& InIdleCounter, int InId)
	: Id(InId), IdleCounter(InIdleCounter)
{
	WorkEvent = FPlatformProcess::GetSynchEventFromPool(false);
	UE_LOG(LogEnigmaVoxelWorker, Warning, TEXT("FChunkWorker [%d] is created"), Id);
}

FChunkWorker::~FChunkWorker()
{
	FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
	WorkEvent = nullptr;
	UE_LOG(LogEnigmaVoxelWorker, Warning, TEXT("FChunkWorker [%d] was deleted"), Id);
}

void FChunkWorker::AssignJob(TFunction<void()>&& InJob)
{
	FScopeLock _(&JobMutex);
	Job   = MoveTemp(InJob);
	bIdle = false;
	IdleCounter.Decrement();
	WorkEvent->Trigger();
}

uint32 FChunkWorker::Run()
{
	while (!bStop)
	{
		WorkEvent->Wait();
		if (bStop)
		{
			break;
		}

		TFunction<void()> LocalJob;
		{
			FScopeLock _(&JobMutex);
			LocalJob = MoveTemp(Job);
		}
		if (LocalJob)
		{
			LocalJob();
		}

		// 还原空闲状态
		bIdle = true;
		IdleCounter.Increment();
	}
	return 0;
}
