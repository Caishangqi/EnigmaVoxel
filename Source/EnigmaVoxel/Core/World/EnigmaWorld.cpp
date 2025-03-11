// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorld.h"

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
	static constexpr int32 ChunkBlockCount = 16;
	static constexpr float BlockWorldSize  = 100.f;

	float ChunkWorldSize = ChunkBlockCount * BlockWorldSize;
	int32 ChunkX         = (int32)FMath::FloorToInt(WorldPos.X / ChunkWorldSize);
	int32 ChunkY         = (int32)FMath::FloorToInt(WorldPos.Y / ChunkWorldSize);

	return FIntVector(ChunkX, ChunkY, 0);
}
