// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorld.h"

#include "EnigmaVoxel/Core/EVGameInstance.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"

UWorld* UEnigmaWorld::GetWorld() const
{
	return CurrentUWorld;
}

bool UEnigmaWorld::SetUWorldTarget(UWorld* UnrealBuildInWorld)
{
	CurrentUWorld = UnrealBuildInWorld;
	if (CurrentUWorld == nullptr)
		return false;
	return true;
}

bool UEnigmaWorld::SetEnableWorldTick(bool Enable)
{
	EnableWorldTick = Enable;
	if (EnableWorldTick)
		return true;
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
}

void UEnigmaWorld::UpdateStreamingChunks()
{
	if (!CurrentUWorld)
	{
		UE_LOG(LogEnigmaVoxelWorld, Warning, TEXT("UEnigmaWorld::UpdateStreamingChunks() - CurrentUWorld is null"))
		return;
	}

	TSet<FIntVector> DesiredChunkCoords;
	const int32      LoadRadius = 3;
	for (APawn* Pawn : Players)
	{
		if (!Pawn) continue;

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
	/// We constantly check the dirty chunk, if we found the dirty chunk we
	/// rebuild their data based on its current blockdata and neibourhood state
	for (auto& KV : ChunkMap)
	{
		FIntVector  ChunkCoords = KV.Key;
		FChunkInfo& Info        = KV.Value;
		if (Info.bIsDirty && Info.LoadState == EChunkLoadState::LOADED)
		{
			Info.bIsDirty = false;
			Async(EAsyncExecution::ThreadPool,
			      [this, ChunkCoords]()
			      {
				      FScopeLock Lock(&ChunkMapMutex);
				      if (!ChunkMap.Contains(ChunkCoords))
					      return;
				      FChunkInfo& InfoRef = ChunkMap[ChunkCoords];
				      RebuildChunkMeshData(InfoRef);
			      },
			      [this, ChunkCoords]() // Update the chunk mesh data in the main thread
			      {
				      AsyncTask(ENamedThreads::GameThread, [this, ChunkCoords]()
				      {
					      FScopeLock Lock(&ChunkMapMutex);
					      if (!ChunkMap.Contains(ChunkCoords))
						      return;
					      FChunkInfo& InfoRef = ChunkMap[ChunkCoords];
					      if (!LoadedChunks.Contains(ChunkCoords))
						      return;
					      AChunk*        chunk   = LoadedChunks[ChunkCoords];
					      UDynamicMesh*  DynMesh = chunk->GetDynamicMeshComponent()->GetDynamicMesh();
					      FDynamicMesh3& MeshRef = DynMesh->GetMeshRef();
					      MeshRef                = InfoRef.ChunkData.BuiltMeshData.Mesh;
					      chunk->GetDynamicMeshComponent()->NotifyMeshUpdated();
				      });
			      });
		}
	}

	for (const FIntVector& ChunkCoords : ToLoad)
	{
		BeginLoadChunkAsync(ChunkCoords);
	}

	// TODO: Consider implement UnloadChunkAsync() if needed
	for (const FIntVector& ChunkCoords : ToUnload)
	{
		UnloadChunk(ChunkCoords);
	}
}

AChunk* UEnigmaWorld::LoadChunk(const FIntVector& ChunkCoords)
{
	if (!CurrentUWorld) return nullptr;

	if (LoadedChunks.Contains(ChunkCoords))
	{
		return LoadedChunks[ChunkCoords];
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FVector ChunkOrigin((float)(ChunkCoords.X) * 1600.f, (float)(ChunkCoords.Y) * 1600.f, (float)(ChunkCoords.Z) * 1600.f);
	AChunk* NewChunkActor = CurrentUWorld->SpawnActor<AChunk>(AChunk::StaticClass(), ChunkOrigin, FRotator::ZeroRotator, SpawnParams);
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
	FScopeLock Lock(&ChunkMapMutex);

	if (!ChunkMap.Contains(ChunkCoords))
	{
		return false;
	}

	// If the Actor already exists, Destroy it
	if (AChunk* FoundActor = LoadedChunks.FindRef(ChunkCoords))
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
	FChunkData&   ChunkData = InOutChunkInfo.ChunkData;
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
				// TODO: Try to implement the chunk border block visible when other chunk
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
	{
		FScopeLock Lock(&ChunkMapMutex);
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
			if (Info.LoadState != EChunkLoadState::UNLOADED)
			{
				// If it is already loading or has been loaded, do not repeat the request
				return;
			}
			Info.LoadState = EChunkLoadState::LOADING;
		}
	}


	// Generate asynchronously in background thread
	Async(EAsyncExecution::ThreadPool, [this, ChunkCoords]()
	      {
		      // Background thread needs to read or write ChunkMap[ChunkCoords].ChunkData => need lock
		      {
			      FScopeLock Lock(&ChunkMapMutex);
			      if (!ChunkMap.Contains(ChunkCoords))
			      {
				      return;
			      }
			      FChunkInfo& Info = ChunkMap[ChunkCoords];
			      GenerateChunkDataAsync(Info.ChunkData); // We calculate the Chunk Mesh
		      }
	      }, [this, ChunkCoords]() // After background thread complete we need the data to spawn actual Chunk Actor
	      {
		      AsyncTask(ENamedThreads::GameThread, [this, ChunkCoords]()
		      {
			      FScopeLock Lock(&ChunkMapMutex);

			      // Chunk may have been unloaded while executing in the background => Check
			      if (!ChunkMap.Contains(ChunkCoords))
				      return;

			      FChunkInfo& Info = ChunkMap[ChunkCoords];
			      if (Info.LoadState != EChunkLoadState::LOADING)
				      return; // May be marked as UNLOADED during background operation
			      Info.LoadState = EChunkLoadState::LOADED;
			      /// Prepare spawn Chunk Actor
			      FActorSpawnParameters SpawnParams;
			      SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			      FVector ChunkOrigin(float(ChunkCoords.X) * 1600.f, float(ChunkCoords.Y) * 1600.f, 0.f);
			      AChunk* NewChunkActor = CurrentUWorld->SpawnActor<AChunk>(AChunk::StaticClass(), ChunkOrigin, FRotator::ZeroRotator, SpawnParams);

			      if (!NewChunkActor)
				      return;
			      ///

			      // Assign the Mesh generated by the background thread to AChunk->DynamicMeshComponent
			      {
				      UDynamicMesh*  DynMesh = NewChunkActor->GetDynamicMeshComponent()->GetDynamicMesh();
				      FDynamicMesh3& MeshRef = DynMesh->GetMeshRef();

				      // Move the background data to this MeshRef
				      // MoveTemp can avoid copying, but make sure you don't use Info.ChunkData.BuiltMeshData.Mesh again
				      MeshRef = Info.ChunkData.BuiltMeshData.Mesh;
				      NewChunkActor->GetDynamicMeshComponent()->NotifyMeshUpdated();
				      // NewChunkActor->GetDynamicMeshComponent()->UpdateCollision();
			      }

			      // Update LoadedChunksMap (or LoadedChunks) to manage
			      LoadedChunks.Add(ChunkCoords, NewChunkActor);
			      NotifyNeighborsChunkLoaded(ChunkCoords);
			      UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("Loaded Chunk at ChunkPos= [ %s ]"), *ChunkCoords.ToString())
		      });
	      });
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

FIntVector UEnigmaWorld::WorldPosToChunkCoords(const FVector& WorldPos)
{
	int32 ChunkX = (int32)FMath::FloorToInt(WorldPos.X / ChunkWorldSize);
	int32 ChunkY = (int32)FMath::FloorToInt(WorldPos.Y / ChunkWorldSize);
	return FIntVector(ChunkX, ChunkY, 0);
}

FIntVector UEnigmaWorld::BlockPosToChunkCoords(const FIntVector& BlockPos)
{
	return WorldPosToChunkCoords(FVector((float)BlockPos.X * BlockWorldSize, (float)BlockPos.Y * BlockWorldSize, (float)BlockPos.Z * BlockWorldSize));
}

FIntVector UEnigmaWorld::WorldPosToChunkLocalCoords(const FVector& WorldPos)
{
	FIntVector chunkCoords = WorldPosToChunkCoords(WorldPos);
	// 计算 chunkWorldOrigin = (chunkCoords * ChunkWorldSize)
	FVector chunkWorldOrigin(float(chunkCoords.X) * ChunkWorldSize, float(chunkCoords.Y) * ChunkWorldSize, 0.f);
	// 然后 (WorldPos - chunkWorldOrigin) / BlockWorldSize 就是相对于 chunk 的本地 block 索引
	float localBlockX = (float)(WorldPos.X - chunkWorldOrigin.X) / BlockWorldSize;
	float localBlockY = (float)(WorldPos.Y - chunkWorldOrigin.Y) / BlockWorldSize;
	float localBlockZ = (float)(WorldPos.Z - chunkWorldOrigin.Z) / BlockWorldSize;

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
	return WorldPosToChunkLocalCoords(FVector((float)BlockPos.X * BlockWorldSize, (float)BlockPos.Y * BlockWorldSize, (float)BlockPos.Z * BlockWorldSize));
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
	return GetBlockAtWorldPos(FVector((float)BlockPos.X * BlockWorldSize, (float)BlockPos.Y * BlockWorldSize, (float)BlockPos.Z * BlockWorldSize));
}
