// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "EnigmaVoxel/Core/Register/RegistrationSubsystem.h"


// Sets default values
AChunk::AChunk()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionDynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Collision Dynamic Mesh"));

	// Reserve the block slot
	Blocks.Reserve(GetChunkBlockSize());
	Blocks.Init(FBlock(), GetChunkBlockSize());
}

// Called when the game starts or when spawned
void AChunk::BeginPlay()
{
	Super::BeginPlay();
}

int32 AChunk::GetChunkBlockSize()
{
	return ChunkDimension.X * ChunkDimension.Y * ChunkDimension.Z;
}

FBlock& AChunk::GetBlockAt(FIntVector InCoords)
{
	int32 Index = GetBlockIndexAt(InCoords);
	check(Blocks.IsValidIndex(Index));
	return Blocks[Index];
}

int32 AChunk::GetBlockIndexAt(FIntVector InCoords)
{
	check(InCoords.X >= 0 && InCoords.X < ChunkDimension.X);
	check(InCoords.Y >= 0 && InCoords.Y < ChunkDimension.Y);
	check(InCoords.Z >= 0 && InCoords.Z < ChunkDimension.Z);

	return InCoords.X
		+ InCoords.Y * ChunkDimension.X
		+ InCoords.Z * ChunkDimension.X * ChunkDimension.Y;
}

bool AChunk::UpdateBlock(FBlock InBlockData)
{
	Blocks[GetBlockIndexAt(InBlockData.Coordinates)] = InBlockData;
	UE_LOG(LogEnigmaVoxelChunk, Log, TEXT("UpdateBlock in Chunk -> %s at -> %s"), *GetName(), *InBlockData.Coordinates.ToString())
	if (InBlockData.Definition == nullptr)
	{
		return false;
	}
	return true;
}

bool AChunk::UpdateBlockResourceLocation(FIntVector InCoords, FString Namespace, FString Path)
{
	UBlockDefinition* def   = URegistrationSubsystem::BLOCK_GET_VALUE(Namespace, Path);
	FBlock            Block = FBlock(InCoords, def, 100);
	return UpdateBlock(Block);
}

bool AChunk::UpdateChunk()
{
	if (!DynamicMeshComponent)
	{
		UE_LOG(LogEnigmaVoxelChunk, Error, TEXT("Fail to get DynamicMeshComponent in Chunk -> %s"), *GetName())
		return false;
	}
	TObjectPtr<UDynamicMesh> DynamicMesh = NewObject<UDynamicMesh>();
	if (!DynamicMesh)
	{
		UE_LOG(LogEnigmaVoxelChunk, Error, TEXT("Fail to create new dynamic mesh in Chunk -> %s"), *GetName());
		return false;
	}
	DynamicMeshComponent->GetDynamicMesh()->Reset();
	UE_LOG(LogEnigmaVoxelChunk, Log, TEXT("Successful create new dynamic mesh in Chunk -> %s"), *GetName());
	for (FBlock& Block : Blocks)
	{
		if (Block.Definition != nullptr)
		{
			AppendBoxWithCollision(Block);
		}
	}
	return false;
}

void AChunk::AppendBoxWithCollision(FBlock& Block)
{
	FIntVector blockPos          = Block.Coordinates;
	FVector    blockVertMinPoint = FVector(blockPos.X * 100, blockPos.Y * 100, blockPos.Z * 100);
	FVector    blockVertMaxPoint = FVector(blockVertMinPoint.X + 100, blockVertMinPoint.Y + 100, blockVertMinPoint.Z + 100);
	FBox3d     box               = FBox3d(blockVertMinPoint, blockVertMaxPoint);
	FVector3d  BoxMin            = box.Min;
	FVector3d  BoxMax            = box.Max;

	// +x faces
	UDynamicMesh*  DynamicMesh = DynamicMeshComponent->GetDynamicMesh();
	FDynamicMesh3& mesh        = DynamicMesh->GetMeshRef();
	int32          v0          = mesh.AppendVertex(FVector(BoxMax.X, BoxMin.Y, BoxMin.Z));
	int32          v1          = mesh.AppendVertex(FVector(BoxMax.X, BoxMax.Y, BoxMin.Z));
	int32          v2          = mesh.AppendVertex(FVector(BoxMax.X, BoxMax.Y, BoxMax.Z));
	int32          v3          = mesh.AppendVertex(FVector(BoxMax.X, BoxMin.Y, BoxMax.Z));
	mesh.AppendTriangle(v0, v1, v2);
	mesh.AppendTriangle(v0, v2, v3);

	// -x faces
	int32 v4 = mesh.AppendVertex(FVector(BoxMin.X, BoxMax.Y, BoxMin.Z));
	int32 v5 = mesh.AppendVertex(FVector(BoxMin.X, BoxMin.Y, BoxMin.Z));
	int32 v6 = mesh.AppendVertex(FVector(BoxMin.X, BoxMin.Y, BoxMax.Z));
	int32 v7 = mesh.AppendVertex(FVector(BoxMin.X, BoxMax.Y, BoxMax.Z));
	mesh.AppendTriangle(v5, v4, v7);
	mesh.AppendTriangle(v5, v7, v6);

	// -y faces
	mesh.AppendTriangle(v4, v1, v2);
	mesh.AppendTriangle(v4, v2, v7);

	// +y faces
	mesh.AppendTriangle(v0, v5, v6);
	mesh.AppendTriangle(v0, v6, v3);

	// -z faces
	mesh.AppendTriangle(v1, v0, v5);
	mesh.AppendTriangle(v1, v5, v4);

	// +z faces
	mesh.AppendTriangle(v2, v3, v6);
	mesh.AppendTriangle(v2, v6, v7);

	DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DynamicMeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	DynamicMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	DynamicMeshComponent->SetDeferredCollisionUpdatesEnabled(true);
	DynamicMeshComponent->bEnableComplexCollision = true;

	DynamicMeshComponent->SetDynamicMesh(DynamicMesh);
	DynamicMeshComponent->UpdateCollision();

	DynamicMeshComponent->RecreatePhysicsState();
	UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("Append new Block triangles -> %s"), *Block.Coordinates.ToString());
}


// Called every frame
void AChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
