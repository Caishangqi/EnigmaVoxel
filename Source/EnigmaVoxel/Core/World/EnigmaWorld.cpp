// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorld.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "GameFramework/DefaultPawn.h"
#include "Kismet/GameplayStatics.h"

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
				FIntVector NewCoords = FIntVector(
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
		LoadChunk(ChunkCoords);
	}
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
		NewChunkActor->FillChunkWithXYZ(FIntVector(16, 16, 1), TEXT("Enigma"), TEXT("Blue Enigma Block"));
		NewChunkActor->UpdateChunk();
	}

	return NewChunkActor;
}

bool UEnigmaWorld::UnloadChunk(const FIntVector& ChunkCoords)
{
	if (AChunk* chunkActor = LoadedChunks.FindRef(ChunkCoords))
	{
		chunkActor->Destroy();
		LoadedChunks.Remove(ChunkCoords);
		return true;
	}
	return false;
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
	int32 ChunkX         = FMath::FloorToInt(WorldPos.X / ChunkWorldSize);
	int32 ChunkY         = FMath::FloorToInt(WorldPos.Y / ChunkWorldSize);

	return FIntVector(ChunkX, ChunkY, 0);
}
