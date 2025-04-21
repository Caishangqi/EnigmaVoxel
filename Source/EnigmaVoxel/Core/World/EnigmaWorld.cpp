// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorld.h"

#include "EnigmaVoxel/Core/EVGameInstance.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "EnigmaVoxel/Modules/Chunk/ChunkAsyncTask.h"
#include "Thread/ChunkWorkerPool.h"

UWorld* UEnigmaWorld::GetWorld() const
{
	return CurrentUWorld;
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
	static const TArray<FIntVector> Directions = {
		FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0), FIntVector(0, -1, 0)
	};
	for (const FIntVector& Dir : Directions)
	{
		FIntVector neighborCoords = ChunkCoords + Dir;
		if (FChunkInfo* NeighborInfoPtr = ChunkMap.Find(neighborCoords))
		{
			// If the neighbor is in the LOADED state, mark it as dirty
			if (NeighborInfoPtr->LoadState == EChunkLoadState::LOADED)
			{
				NeighborInfoPtr->bIsDirty = true;
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
		ChunkWorkerPool->RequestExit(); // 标记 + Trigger
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

	TSet<FIntVector> DesiredChunkCoords;
	constexpr int32  LoadRadius = 3;
	for (APawn* Pawn : Players)
	{
		if (!Pawn)
		{
			continue;
		}

		FVector    PawnLocation      = Pawn->GetActorLocation();
		FIntVector CenterChunkCoords = WorldPosToChunkCoords(PawnLocation);

		for (int32 dx = -LoadRadius; dx <= LoadRadius; ++dx)
		{
			for (int32 dy = -LoadRadius; dy <= LoadRadius; ++dy)
			{
				FIntVector NewCoords(
					CenterChunkCoords.X + dx,
					CenterChunkCoords.Y + dy,
					CenterChunkCoords.Z
				);
				DesiredChunkCoords.Add(NewCoords);
			}
		}
	}

	TSet<FIntVector> ExistingChunks;
	LoadedChunks.GetKeys(ExistingChunks);

	TSet<FIntVector> ToLoad   = DesiredChunkCoords.Difference(ExistingChunks);
	TSet<FIntVector> ToUnload = ExistingChunks.Difference(DesiredChunkCoords);


	/*FAsyncTask<FChunkAsyncTask>* chunkTask = new FAsyncTask<FChunkAsyncTask>(this, Load);
	chunkTask->StartBackgroundTask(ChunkWorkerThreadPool.Get());*/

	/// We constantly check the dirty chunk, if we found the dirty chunk we
	/// rebuild their data based on its current blockdata and neibourhood state

	/*TMap<FIntVector, FChunkInfo> TempMap = CopyTemp(ChunkMap);
	for (int i = 0; i < TempMap.Num(); ++i)
	{
		TArray<FIntVector> ChunkCoords;
		TempMap.GetKeys(ChunkCoords);
		UpdateChunkAsync(ChunkCoords[i]);
	}*/

	/*TSet<FIntVector> ToLoadTemp = CopyTemp(ToLoad);
	for (const FIntVector& ChunkCoords : ToLoadTemp)
	{
		BeginLoadChunkAsync(ChunkCoords);
	}

	// TODO: Consider implement UnloadChunkAsync() if needed
	for (const FIntVector& ChunkCoords : ToUnload)
	{
		UnloadChunk(ChunkCoords);
	}*/
}

AChunkActor* UEnigmaWorld::LoadChunk(const FIntVector& ChunkCoords)
{
	if (!CurrentUWorld)
	{
		return nullptr;
	}

	if (LoadedChunks.Contains(ChunkCoords))
	{
		return LoadedChunks[ChunkCoords];
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FVector      ChunkOrigin(static_cast<float>(ChunkCoords.X) * 1600.f, static_cast<float>(ChunkCoords.Y) * 1600.f, static_cast<float>(ChunkCoords.Z) * 1600.f);
	AChunkActor* NewChunkActor = CurrentUWorld->SpawnActor<AChunkActor>(AChunkActor::StaticClass(), ChunkOrigin, FRotator::ZeroRotator, SpawnParams);
	if (NewChunkActor)
	{
		LoadedChunks.Add(ChunkCoords, NewChunkActor);
		NewChunkActor->FillChunkWithXYZ(FIntVector(16, 16, 8), TEXT("Enigma"), TEXT("Blue Enigma Block"));
		NewChunkActor->UpdateChunk();
	}

	return NewChunkActor;
}

bool UEnigmaWorld::UnloadChunk(const FIntVector& ChunkCoords)
{
	if (!ChunkMap.Contains(ChunkCoords))
	{
		return false;
	}

	// If the Actor already exists, Destroy it
	if (AChunkActor* FoundActor = LoadedChunks.FindRef(ChunkCoords))
	{
		FoundActor->Destroy();
		LoadedChunks.Remove(ChunkCoords);
	}

	// Remove ChunkMap
	ChunkMap.Remove(ChunkCoords);
	UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("Unloaded Chunk at ChunkPos= [ %s ]"), *ChunkCoords.ToString())
	return true;
}

void UEnigmaWorld::RebuildChunkMeshData(FChunkInfo& InOutChunkInfo)
{
	FChunkData& ChunkData = InOutChunkInfo.ChunkData;
	ChunkData.RefreshMaterialData();
	FDynamicMesh3 TempMesh;

	UE_LOG(LogEnigmaVoxelChunk, Warning, TEXT("Rebuild Chunk at ChunkPos= [ %s ]"), *ChunkData.ChunkCoords.ToString())

	for (int z = 0; z < ChunkData.ChunkDimension.Z; ++z)
	{
		for (int y = 0; y < ChunkData.ChunkDimension.Y; ++y)
		{
			for (int x = 0; x < ChunkData.ChunkDimension.X; ++x)
			{
				const FBlock& Block = ChunkData.GetBlock(FIntVector(x, y, z));
				if (!Block.Definition)
				{
					continue;
				}
				AppendBoxForBlock(this, TempMesh, Block, ChunkData);
			}
		}
	}
	ChunkData.BuiltMeshData.Mesh = TempMesh;
	ChunkData.bHasBuiltMesh      = true;
	UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("FChunkData Mesh at [ %s ] Mesh Build successful"), *ChunkData.ChunkCoords.ToString())
}

void UEnigmaWorld::GenerateChunkDataAsync(FChunkData& InOutChunkData)
{
	// Fill the block data ( This is a simple example) this method will finally integrate
	// with perlin noise
	InOutChunkData.FillChunkWithArea(FIntVector(16, 16, 8), "Enigma", "Blue Enigma Block");

	FDynamicMesh3 TempMesh;

	/// TODO: Consider move the Material Section Cache here that maintain ChunkData as pure data
	/// TODO: Fix the concurrent issue that make the Material section chaose.
	for (int z = 0; z < InOutChunkData.ChunkDimension.Z; ++z)
	{
		for (int y = 0; y < InOutChunkData.ChunkDimension.Y; ++y)
		{
			for (int x = 0; x < InOutChunkData.ChunkDimension.X; ++x)
			{
				const FBlock& Block = InOutChunkData.GetBlock(FIntVector(x, y, z));
				if (!Block.Definition)
				{
					continue;
				}
				// Append the visible surface of the block to TempMesh
				// UNLOADED, but culling after other LOADED
				AppendBoxForBlock(TempMesh, Block, InOutChunkData);
			}
		}
	}
	InOutChunkData.BuiltMeshData.Mesh = TempMesh;
	InOutChunkData.bHasBuiltMesh      = true;
	UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("FChunkData Mesh at [ %s ] Mesh Build successful"), *InOutChunkData.ChunkCoords.ToString())
}

void UEnigmaWorld::BeginLoadChunkAsync(const FIntVector& ChunkCoords)
{
	// Generate asynchronously in background thread
	Async(EAsyncExecution::ThreadPool, [this, ChunkCoords]()
	      {
		      ChunkMapMutex.Lock();
		      if (!ChunkMap.Contains(ChunkCoords))
		      {
			      FChunkInfo NewInfo;
			      NewInfo.ChunkData.ChunkCoords    = ChunkCoords;
			      NewInfo.ChunkData.ChunkDimension = FIntVector(16, 16, 16);
			      NewInfo.ChunkData.BlockSize      = 100.f;
			      NewInfo.LoadState                = EChunkLoadState::LOADING;
			      ChunkMap.Add(ChunkCoords, NewInfo);
			      UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("FChunkData Construct: ChunkPos= [ %s ], NumBlocks= [ %d ]"), *ChunkCoords.ToString(), NewInfo.ChunkData.Blocks.Num())
		      }
		      else
		      {
			      FChunkInfo& Info = ChunkMap[ChunkCoords];
			      if (Info.LoadState == EChunkLoadState::LOADING)
			      {
				      ChunkMapMutex.Unlock();
				      return;
			      }
			      if (Info.LoadState != EChunkLoadState::UNLOADED)
			      {
				      // If it is already loading or has been loaded, do not repeat the request
				      ChunkMapMutex.Unlock();
				      return;
			      }
			      Info.LoadState = EChunkLoadState::LOADING;
		      }
		      // Background thread needs to read or write ChunkMap[ChunkCoords].ChunkData => need lock

		      if (!ChunkMap.Contains(ChunkCoords))
		      {
			      return;
		      }
		      GenerateChunkDataAsync(ChunkMap[ChunkCoords].ChunkData); // We calculate the Chunk Mesh
		      ChunkMapMutex.Unlock();
	      },
	      [this, ChunkCoords]() // After background thread complete we need the data to spawn actual Chunk Actor
	      {
		      AsyncTask(ENamedThreads::GameThread, [this, ChunkCoords]()
		      {
			      // Chunk may have been unloaded while executing in the background => Check
			      if (!ChunkMap.Contains(ChunkCoords))
			      {
				      return;
			      }

			      FChunkInfo& Info = ChunkMap[ChunkCoords];
			      if (Info.LoadState != EChunkLoadState::LOADING)
			      {
				      return; // May be marked as UNLOADED during background operation
			      }
			      Info.LoadState = EChunkLoadState::LOADED;
			      /// Prepare spawn Chunk Actor
			      FActorSpawnParameters SpawnParams;
			      SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			      FVector      ChunkOrigin(static_cast<float>(ChunkCoords.X) * 1600.f, static_cast<float>(ChunkCoords.Y) * 1600.f, 0.f);
			      AChunkActor* NewChunkActor = CurrentUWorld->SpawnActor<AChunkActor>(AChunkActor::StaticClass(), ChunkOrigin, FRotator::ZeroRotator, SpawnParams);

			      if (!NewChunkActor)
			      {
				      return;
			      }
			      ///

			      // Assign the Mesh generated by the background thread to AChunkActor->DynamicMeshComponent

			      UDynamicMesh*  DynMesh = NewChunkActor->GetDynamicMeshComponent()->GetDynamicMesh();
			      FDynamicMesh3& MeshRef = DynMesh->GetMeshRef();

			      // Move the background data to this MeshRef
			      // MoveTemp can avoid copying, but make sure you don't use Info.ChunkData.BuiltMeshData.Mesh again
			      MeshRef = CopyTemp(Info.ChunkData.BuiltMeshData.Mesh);
			      NewChunkActor->UpdateChunkMaterial(Info.ChunkData);
			      NewChunkActor->GetDynamicMeshComponent()->NotifyMeshUpdated();
			      // NewChunkActor->GetDynamicMeshComponent()->UpdateCollision();

			      // Update LoadedChunksMap (or LoadedChunks) to manage
			      LoadedChunks.Add(ChunkCoords, NewChunkActor);
			      NotifyNeighborsChunkLoaded(ChunkCoords);
			      UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("Loaded Chunk at ChunkPos= [ %s ]"), *ChunkCoords.ToString())
		      });
	      });
}

void UEnigmaWorld::UpdateChunkAsync(const FIntVector& ChunkCoords)
{
	FChunkInfo Info = ChunkMap[ChunkCoords];
	if (Info.bIsDirty && Info.LoadState == EChunkLoadState::LOADED)
	{
		Async(EAsyncExecution::ThreadPool,
		      [this, ChunkCoords]()
		      {
			      ChunkMapMutex.Lock();
			      if (!ChunkMap.Contains(ChunkCoords))
			      {
				      return;
			      }
			      FChunkInfo& InfoRef = ChunkMap[ChunkCoords];
			      InfoRef.bIsDirty    = false;
			      RebuildChunkMeshData(InfoRef);
			      ChunkMapMutex.Unlock();
		      },
		      [this, ChunkCoords]() // Update the chunk mesh data in the main thread
		      {
			      AsyncTask(ENamedThreads::GameThread, [this, ChunkCoords]()
			      {
				      if (!ChunkMap.Contains(ChunkCoords))
				      {
					      return;
				      }
				      FChunkInfo& InfoRef = ChunkMap[ChunkCoords];
				      if (!LoadedChunks.Contains(ChunkCoords))
				      {
					      return;
				      }
				      AChunkActor*   chunk   = LoadedChunks[ChunkCoords];
				      UDynamicMesh*  DynMesh = chunk->GetDynamicMeshComponent()->GetDynamicMesh();
				      FDynamicMesh3& MeshRef = DynMesh->GetMeshRef();
				      MeshRef                = InfoRef.ChunkData.BuiltMeshData.Mesh;
				      chunk->UpdateChunkMaterial(InfoRef.ChunkData);
				      chunk->GetDynamicMeshComponent()->NotifyMeshUpdated();
			      });
		      });
	}
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
		ChunkWorkerPool->RequestExit(); // 让线程自退
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
	if (!ChunkMap.Contains(chunkCoords))
	{
		return nullptr; // 表示此处是“空气”或区块不存在
	}
	FChunkInfo& info = ChunkMap[chunkCoords];
	if (info.LoadState != EChunkLoadState::LOADED)
	{
		return nullptr; // 区块还没加载完，也当成空气处理
	}

	// 获取在这个区块内的局部坐标
	FIntVector localCoords = WorldPosToChunkLocalCoords(WorldPos);
	// 这里最好先检查 localCoords 是否都在 [0..15]
	if (localCoords.X < 0 || localCoords.X >= info.ChunkData.ChunkDimension.X ||
		localCoords.Y < 0 || localCoords.Y >= info.ChunkData.ChunkDimension.Y ||
		localCoords.Z < 0 || localCoords.Z >= info.ChunkData.ChunkDimension.Z)
	{
		return nullptr;
	}

	// 获取此处的方块
	const FBlock& blockData = info.ChunkData.GetBlock(localCoords);
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
