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

		if (H->bDirty && !H->bQueuedForRebuild.exchange(true))
		{
			H->bDirty = false;
			ChunkWorkerPool->EnqueueBuildTask(H, /*bMeshOnly=*/true, this);
		}

		// Exceed Grace period, destroy.
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

	// Collect the field of view of this tick
	TSet<FIntVector> Desired;
	GatherPlayerVisibleSet(Desired);

	// Add / subtract tickets -> submit task/unload
	ProcessTickets(Desired, Now);

	// Thread pool result → Generate or update Actor
	PumpWorkerResults();

	// Handle bDirty reconstruction & actually destroy the expired PendingUnload block
	FlushDirtyAndPending(Now);

	// Save the collection for next tick difference
	PrevVisibleSet = MoveTemp(Desired);
}

void UEnigmaWorld::ProcessTickets(const TSet<FIntVector>& Desired, double Now)
{
	TSet<FIntVector> ToLoad   = Desired.Difference(PrevVisibleSet);
	TSet<FIntVector> ToUnload = PrevVisibleSet.Difference(Desired);

	FScopeLock _(&ChunksMutex);

	// Add tickets & possibly schedule tasks
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

		if (H->Stage == EChunkStage::Loading)
		{
			ChunkWorkerPool->EnqueueBuildTask(H, false, this);
		}
	}

	// Reduce ticket (leave the field of view once)
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
		if (H->Stage != EChunkStage::Ready)
		{
			continue;
		}

		AsyncTask(ENamedThreads::GameThread, [this, Coords = KV.Key, H]()
		{
			AChunkActor* CA = LoadedChunks.FindRef(Coords);
			if (!CA)
			{
				FActorSpawnParameters P;
				P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				FVector Origin((float)Coords.X * 1600.f, (float)Coords.Y * 1600.f, 0.f); // TODO: Parameterize the statement
				CA = CurrentUWorld->SpawnActor<AChunkActor>(
					AChunkActor::StaticClass(), Origin, FRotator::ZeroRotator, P);
				LoadedChunks.Add(Coords, CA);
			}

			UDynamicMesh* DynMesh = CA->GetDynamicMeshComponent()->GetDynamicMesh();
			DynMesh->GetMeshRef() = CopyTemp(H->Mesh);
			CA->UpdateChunkMaterial(*H);

			H->Stage = EChunkStage::Loaded;
			if (H->bNeedsNeighborNotify.exchange(false, std::memory_order_relaxed))
			{
				NotifyNeighborsChunkLoaded(Coords);
			}
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
			if (N->Stage == EChunkStage::Loaded || N->Stage == EChunkStage::Ready)
			{
				N->bDirty            = true;
				N->bQueuedForRebuild = false;
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
		ChunkWorkerPool->Shutdown(); // Wait for all threads to exit safely
	}
	Super::BeginDestroy();
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
	// Calculate chunkWorldOrigin = (chunkCoords * ChunkWorldSize)
	FVector chunkWorldOrigin(static_cast<float>(chunkCoords.X) * ChunkWorldSize, static_cast<float>(chunkCoords.Y) * ChunkWorldSize, 0.f);
	// Then (WorldPos - chunkWorldOrigin) / BlockWorldSize is the local block index relative to the chunk
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

	// If it does not exist or has not been loaded yet
	if (!Chunks.Contains(chunkCoords))
	{
		return nullptr; // Indicates that this is "air" or the block does not exist
	}
	FChunkHolder* holder = Chunks[chunkCoords].Get();
	if (holder->Stage != EChunkStage::Ready && holder->Stage != EChunkStage::Loaded)
	{
		return nullptr; // The block has not been loaded yet, so it is treated as air.
	}

	// Get the local coordinates within this block
	FIntVector localCoords = WorldPosToChunkLocalCoords(WorldPos);
	// It is best to check if localCoords are all in [0..15]
	if (localCoords.X < 0 || localCoords.X >= holder->Dimension.X ||
		localCoords.Y < 0 || localCoords.Y >= holder->Dimension.Y ||
		localCoords.Z < 0 || localCoords.Z >= holder->Dimension.Z)
	{
		return nullptr;
	}

	// Get the block here
	const FBlock& blockData = holder->GetBlock(localCoords);
	if (!blockData.Definition)
	{
		return nullptr;
	}
	return blockData.Definition;
}

UBlockDefinition* UEnigmaWorld::GetBlockAtBlockPos(const FIntVector& BlockPos)
{
	FScopeLock _(&ChunksMutex);
	return GetBlockAtWorldPos(FVector(static_cast<float>(BlockPos.X) * BlockWorldSize, static_cast<float>(BlockPos.Y) * BlockWorldSize, static_cast<float>(BlockPos.Z) * BlockWorldSize));
}
