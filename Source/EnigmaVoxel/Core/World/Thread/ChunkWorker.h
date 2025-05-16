#pragma once
#include "HAL/Runnable.h"
#include "Templates/Function.h"

class UChunkWorkerPool;

class FChunkWorker : public FRunnable
{
public:
	FChunkWorker(UChunkWorkerPool& InPool, int InId);
	virtual ~FChunkWorker() override;

	bool           IsIdle() const { return bIdle; }
	void           Wake();
	virtual uint32 Run() override;
	virtual void   Stop() override;

private:
	UChunkWorkerPool& Pool;
	int               Id = 0;
	FThreadSafeBool   bStop{false};
	FThreadSafeBool   bIdle{true};

	FEvent* WakeEvent = nullptr;
};
