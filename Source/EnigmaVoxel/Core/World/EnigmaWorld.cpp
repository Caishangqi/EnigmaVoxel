// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorld.h"
#include "EnigmaVoxel/Core/EVGameInstance.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "EnigmaVoxel/Modules/Chunk/ChunkHolder.h"
#include "Thread/ChunkWorkerPool.h"

UWorld* UEnigmaWorld::GetWorld() const
{
	return CurrentUWorld;
}

void UEnigmaWorld::GatherPlayerVisibleSet(TSet<FIntVector>& Out)
{
	for (FConstPlayerControllerIterator It = CurrentUWorld->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* PC = It->Get())
		{
			const APawn* P = PC->GetPawn();
			if (!P)
			{
				continue;
			}

			FIntVector Center = WorldPosToChunkCoords(P->GetActorLocation());
			for (int dy = -ViewRadius; dy <= ViewRadius; ++dy)
			{
				for (int dx = -ViewRadius; dx <= ViewRadius; ++dx)
				{
					Out.Add(Center + FIntVector(dx, dy, 0));
				}
			}
		}
	}
}

void UEnigmaWorld::FlushDirtyAndPending(double Now)
{
	FScopeLock _(&ChunksMutex);

	for (auto It = Chunks.CreateIterator(); It; ++It)
	{
		FChunkHolder* H = It.Value().Get();

		/* --- bDirty -> 重构 --- */
		if (H->Stage == EChunkStage::Ready && H->bDirty.load() && !H->bQueuedForRebuild.exchange(true))
		{
			ChunkWorkerPool->EnqueueBuildTask(H, true); // Mesh only
		}

		/* --- 宽限到期 -> 销毁 --- */
		if (H->Stage == EChunkStage::PendingUnload && H->PendingUnloadUntil < Now)
		{
			if (AChunkActor* CA = LoadedChunks.FindRef(H->Coords))
			{
				CA->Destroy();
				LoadedChunks.Remove(H->Coords);
			}
			It.RemoveCurrent();
		}
	}
}

void UEnigmaWorld::Tick()
{
	if (!CurrentUWorld)
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();

	/* 1) 收集本帧视野 */
	TSet<FIntVector> Desired;
	GatherPlayerVisibleSet(Desired);

	/* 2) 加/减票据 -> 提交任务/卸载 */
	ProcessTickets(Desired, Now);

	/* 3) 线程池结果 → 生成或更新 Actor */
	PumpWorkerResults();

	/* 4) 处理 bDirty 重构 & 真正销毁 PendingUnload 到期的区块 */
	FlushDirtyAndPending(Now);

	/* 5) 保存集合供下帧差集 */
	PrevVisibleSet = MoveTemp(Desired);
}

TSet<FIntVector> UEnigmaWorld::GatherPlayerView(int radius)
{
	TSet<FIntVector> Desired;

	if (radius < 0) // 容错
	{
		return Desired;
	}

	for (APawn* Pawn : Players)
	{
		if (!Pawn)
		{
			continue;
		}

		const FVector    PawnLocation = Pawn->GetActorLocation();
		const FIntVector CenterCoords = WorldPosToChunkCoords(PawnLocation);

		for (int32 dx = -radius; dx <= radius; ++dx)
		{
			for (int32 dy = -radius; dy <= radius; ++dy)
			{
				FIntVector C(CenterCoords.X + dx, CenterCoords.Y + dy, 0);
				Desired.Add(C);
			}
		}
	}
	return Desired;
}

void UEnigmaWorld::ProcessTickets(const TSet<FIntVector>& Desired, double Now)
{
	TSet<FIntVector> ToLoad   = Desired.Difference(PrevVisibleSet);
	TSet<FIntVector> ToUnload = PrevVisibleSet.Difference(Desired);

	FScopeLock _(&ChunksMutex);

	/* ---- 加票 & 可能排任务 ---- */
	for (const FIntVector& C : ToLoad)
	{
		FChunkHolder* H = nullptr;
		if (TUniquePtr<FChunkHolder>* Ptr = Chunks.Find(C))
		{
			H = Ptr->Get();
		}
		else
		{
			H = Chunks.Add(C, MakeUnique<FChunkHolder>()).Get();
		}

		H->Coords = C;
		H->AddTicket();

		if (H->Stage == EChunkStage::Unloaded)
		{
			H->Stage = EChunkStage::Loading;
			ChunkWorkerPool->EnqueueBuildTask(H, false); // 生成块+网格
		}
	}

	/* ---- 减票 (离开视野一次) ---- */
	for (const FIntVector& C : ToUnload)
	{
		if (TUniquePtr<FChunkHolder>* Ptr = Chunks.Find(C))
		{
			(*Ptr)->RemoveTicket(Now, GracePeriod);
		}
	}
}


void UEnigmaWorld::PumpWorkerResults()
{
	FScopeLock _(&ChunksMutex);

	for (auto& KV : Chunks)
	{
		FChunkHolder* H = KV.Value.Get();

		/*if (H->Stage == EChunkStage::Loading && H->BuildFuture.IsValid() && H->BuildFuture->IsReady())
		{
			H->Stage = EChunkStage::Ready;
		}*/

		if (H->Stage != EChunkStage::Ready || LoadedChunks.Contains(KV.Key))
		{
			continue;
		}

		/* —— 在 GameThread 创建/更新 Actor —— */
		AsyncTask(ENamedThreads::GameThread, [this,Coords = KV.Key, H]()
		{
			AChunkActor* CA = nullptr;
			if (TObjectPtr<AChunkActor>* Found = LoadedChunks.Find(Coords))
			{
				CA = *Found;
			}
			else
			{
				FActorSpawnParameters P;
				P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				FVector Origin(Coords.X * 1600.f, Coords.Y * 1600.f, 0.f); // TODO: 坐标换算
				CA = CurrentUWorld->SpawnActor<AChunkActor>(AChunkActor::StaticClass(), Origin, FRotator::ZeroRotator, P);
				LoadedChunks.Add(Coords, CA);
			}
			UDynamicMesh*  DynMesh = CA->GetDynamicMeshComponent()->GetDynamicMesh();
			FDynamicMesh3& MeshRef = DynMesh->GetMeshRef();
			MeshRef                = MoveTemp(H->Mesh);
			CA->UpdateChunkMaterial(*H);
			NotifyNeighborsChunkLoaded(Coords);
		});
	}
}

bool UEnigmaWorld::SetUWorldTarget(UWorld* UnrealBuildInWorld)
{
	CurrentUWorld = UnrealBuildInWorld;
	if (CurrentUWorld == nullptr)
	{
		return false;
	}
	return true;
}

bool UEnigmaWorld::SetEnableWorldTick(bool Enable)
{
	EnableWorldTick = Enable;
	if (EnableWorldTick)
	{
		return true;
	}
	return false;
}

bool UEnigmaWorld::GetEnableWorldTick()
{
	return EnableWorldTick;
}

/// Notify other chunks that are near the loaded chunk.
/// mark them dirty if they are loaded because they need to
/// rebuild the vertices to cull the edge
/// @param ChunkCoords 
void UEnigmaWorld::NotifyNeighborsChunkLoaded(FIntVector ChunkCoords)
{
	static const FIntVector Offsets[6] = {
		{1, 0, 0}, {-1, 0, 0},
		{0, 1, 0}, {0, -1, 0},
		{0, 0, 1}, {0, 0, -1}
	};

	for (const FIntVector& O : Offsets)
	{
		if (TUniquePtr<FChunkHolder>* Ptr = Chunks.Find(ChunkCoords + O))
		{
			FChunkHolder* N = Ptr->Get();
			if (N->Stage == EChunkStage::Ready)
			{
				N->bDirty            = true;
				N->bQueuedForRebuild = false; // 允许重新排 MeshOnly
			}
		}
	}
}

UEnigmaWorld::UEnigmaWorld()
{
	InitializeChunkWorkerPool();
}

void UEnigmaWorld::BeginDestroy()
{
	if (ChunkWorkerPool)
	{
		ChunkWorkerPool->Shutdown(); // 等待全部线程安全退出
	}
	Super::BeginDestroy();
}


void UEnigmaWorld::UpdateStreamingChunks()
{
	if (!CurrentUWorld)
	{
		UE_LOG(LogEnigmaVoxelWorld, Warning, TEXT("UEnigmaWorld::UpdateStreamingChunks() - CurrentUWorld is null"))
		return;
	}
	const double Now = FPlatformTime::Seconds();
	Tick();
}

bool UEnigmaWorld::AddEntity(APawn* InEntity)
{
	if (InEntity)
	{
		Players.Add(InEntity);
		return true;
	}
	return false;
}

bool UEnigmaWorld::RemoveEntity(APawn* InEntity)
{
	int numOfRemove = Players.Remove(InEntity);
	if (numOfRemove == 0)
	{
		return false;
	}
	return true;
}

void UEnigmaWorld::InitializeChunkWorkerPool()
{
	ChunkWorkerPool = NewObject<UChunkWorkerPool>(this, "ChunkWorkerPool");
	ChunkWorkerPool->Init(6);
}

void UEnigmaWorld::ShutdownChunkWorkerPool()
{
	if (ChunkWorkerPool)
	{
		ChunkWorkerPool->Shutdown(); // Join
	}
}

FIntVector UEnigmaWorld::WorldPosToChunkCoords(const FVector& WorldPos)
{
	int32 ChunkX = static_cast<int32>(FMath::FloorToInt(WorldPos.X / ChunkWorldSize));
	int32 ChunkY = static_cast<int32>(FMath::FloorToInt(WorldPos.Y / ChunkWorldSize));
	return FIntVector(ChunkX, ChunkY, 0);
}

FIntVector UEnigmaWorld::BlockPosToChunkCoords(const FIntVector& BlockPos)
{
	return WorldPosToChunkCoords(FVector(static_cast<float>(BlockPos.X) * BlockWorldSize, static_cast<float>(BlockPos.Y) * BlockWorldSize, static_cast<float>(BlockPos.Z) * BlockWorldSize));
}

FIntVector UEnigmaWorld::WorldPosToChunkLocalCoords(const FVector& WorldPos)
{
	FIntVector chunkCoords = WorldPosToChunkCoords(WorldPos);
	// 计算 chunkWorldOrigin = (chunkCoords * ChunkWorldSize)
	FVector chunkWorldOrigin(static_cast<float>(chunkCoords.X) * ChunkWorldSize, static_cast<float>(chunkCoords.Y) * ChunkWorldSize, 0.f);
	// 然后 (WorldPos - chunkWorldOrigin) / BlockWorldSize 就是相对于 chunk 的本地 block 索引
	float localBlockX = static_cast<float>(WorldPos.X - chunkWorldOrigin.X) / BlockWorldSize;
	float localBlockY = static_cast<float>(WorldPos.Y - chunkWorldOrigin.Y) / BlockWorldSize;
	float localBlockZ = static_cast<float>(WorldPos.Z - chunkWorldOrigin.Z) / BlockWorldSize;

	int32 lx = FMath::FloorToInt(localBlockX); // 0 ~ 15
	int32 ly = FMath::FloorToInt(localBlockY); // 0 ~ 15
	int32 lz = FMath::FloorToInt(localBlockZ); // 0 ~ 15

	auto modChunk = [=](int32 v) { return (v % ChunkBlockXCount + ChunkBlockYCount) % ChunkBlockZCount; };

	lx = modChunk(lx);
	ly = modChunk(ly);
	lz = modChunk(lz);

	return FIntVector(lx, ly, lz);
}

FIntVector UEnigmaWorld::BlockPosToChunkLocalCoords(const FIntVector& BlockPos)
{
	return WorldPosToChunkLocalCoords(FVector(static_cast<float>(BlockPos.X) * BlockWorldSize, static_cast<float>(BlockPos.Y) * BlockWorldSize, static_cast<float>(BlockPos.Z) * BlockWorldSize));
}

UBlockDefinition* UEnigmaWorld::GetBlockAtWorldPos(const FVector& WorldPos)
{
	FIntVector chunkCoords = WorldPosToChunkCoords(WorldPos);

	// 如果不存在或尚未加载
	if (!Chunks.Contains(chunkCoords))
	{
		return nullptr; // 表示此处是“空气”或区块不存在
	}
	FChunkHolder* holder = Chunks[chunkCoords].Get();
	if (holder->Stage != EChunkStage::Ready)
	{
		return nullptr; // 区块还没加载完，也当成空气处理
	}

	// 获取在这个区块内的局部坐标
	FIntVector localCoords = WorldPosToChunkLocalCoords(WorldPos);
	// 这里最好先检查 localCoords 是否都在 [0..15]
	if (localCoords.X < 0 || localCoords.X >= holder->Dimension.X ||
		localCoords.Y < 0 || localCoords.Y >= holder->Dimension.Y ||
		localCoords.Z < 0 || localCoords.Z >= holder->Dimension.Z)
	{
		return nullptr;
	}

	// 获取此处的方块
	const FBlock& blockData = holder->GetBlock(localCoords);
	if (!blockData.Definition)
	{
		return nullptr;
	}
	return blockData.Definition;
}

UBlockDefinition* UEnigmaWorld::GetBlockAtBlockPos(const FIntVector& BlockPos)
{
	return GetBlockAtWorldPos(FVector(static_cast<float>(BlockPos.X) * BlockWorldSize, static_cast<float>(BlockPos.Y) * BlockWorldSize, static_cast<float>(BlockPos.Z) * BlockWorldSize));
}
