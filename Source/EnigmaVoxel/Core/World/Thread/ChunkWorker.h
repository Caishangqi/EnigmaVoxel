#pragma once

class FChunkWorker : public FRunnable
{
public:
	explicit FChunkWorker(FThreadSafeCounter& InIdleCounter, int InId);
	virtual  ~FChunkWorker() override;

	void AssignJob(TFunction<void()>&& InJob);
	bool IsIdle() const { return bIdle; }

	virtual uint32 Run() override;

	virtual void Stop() override
	{
		bStop = true;
		WorkEvent->Trigger();
	}

private:
	TFunction<void()> Job;
	FCriticalSection  JobMutex;

	int Id = 0;

	FThreadSafeBool bIdle{true};
	FThreadSafeBool bStop{false};

	FEvent*             WorkEvent = nullptr;
	FThreadSafeCounter& IdleCounter;
};
