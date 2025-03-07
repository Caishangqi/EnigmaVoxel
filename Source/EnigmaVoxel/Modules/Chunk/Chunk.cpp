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
	//check(Blocks.IsValidIndex(Index));
	return Blocks[Index];
}

int32 AChunk::GetBlockIndexAt(FIntVector InCoords)
{
	/*
	check(InCoords.X >= 0 && InCoords.X < ChunkDimension.X);
	check(InCoords.Y >= 0 && InCoords.Y < ChunkDimension.Y);
	check(InCoords.Z >= 0 && InCoords.Z < ChunkDimension.Z);
	*/

	return InCoords.X
		+ InCoords.Y * ChunkDimension.X
		+ InCoords.Z * ChunkDimension.X * ChunkDimension.Y;
}

bool AChunk::IsVisibleFace(FBlock& currentBlock, EBlockDirection InDirection)
{
	FIntVector3 blockPos = currentBlock.Coordinates;
	switch (InDirection)
	{
	case EBlockDirection::NORTH:
		if (blockPos.X + 1 >= ChunkDimension.X)
			return true;
		blockPos.X += 1;
		if (GetBlockAt(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::SOUTH:
		if (blockPos.X - 1 < 0)
			return true;
		blockPos.X -= 1;
		if (GetBlockAt(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::EAST:
		if (blockPos.Y + 1 >= ChunkDimension.Y)
			return true;
		blockPos.Y += 1;
		if (GetBlockAt(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::WEST:
		if (blockPos.Y - 1 < 0)
			return true;
		blockPos.Y -= 1;
		if (GetBlockAt(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::UP:
		if (blockPos.Z + 1 >= ChunkDimension.Z)
			return true;
		blockPos.Z += 1;
		if (GetBlockAt(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::DOWN:
		if (blockPos.Z - 1 < 0)
			return true;
		blockPos.Z -= 1;
		if (GetBlockAt(blockPos).Definition == nullptr)
			return true;
		return false;
	}
	return false;
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

bool AChunk::FillChunkWithXYZ(FIntVector fillArea, FString Namespace, FString Path)
{
	for (int z = 0; z < fillArea.Z; z++)
	{
		for (int y = 0; y < fillArea.Y; y++)
		{
			for (int x = 0; x < fillArea.X; x++)
			{
				UpdateBlockResourceLocation(FIntVector(x, y, z), Namespace, Path);
			}
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

	UDynamicMesh*  DynamicMesh = DynamicMeshComponent->GetDynamicMesh();
	FDynamicMesh3& mesh        = DynamicMesh->GetMeshRef();

	int32 v0 = mesh.AppendVertex(FVector(BoxMax.X, BoxMin.Y, BoxMin.Z));
	int32 v1 = mesh.AppendVertex(FVector(BoxMax.X, BoxMax.Y, BoxMin.Z));
	int32 v2 = mesh.AppendVertex(FVector(BoxMax.X, BoxMax.Y, BoxMax.Z));
	int32 v3 = mesh.AppendVertex(FVector(BoxMax.X, BoxMin.Y, BoxMax.Z));
	int32 v4 = mesh.AppendVertex(FVector(BoxMin.X, BoxMax.Y, BoxMin.Z));
	int32 v5 = mesh.AppendVertex(FVector(BoxMin.X, BoxMin.Y, BoxMin.Z));
	int32 v6 = mesh.AppendVertex(FVector(BoxMin.X, BoxMin.Y, BoxMax.Z));
	int32 v7 = mesh.AppendVertex(FVector(BoxMin.X, BoxMax.Y, BoxMax.Z));

	// +x faces
	if (IsVisibleFace(Block, EBlockDirection::NORTH))
	{
		mesh.AppendTriangle(v1, v0, v3);
		mesh.AppendTriangle(v1, v3, v2);
	}

	// -x faces
	if (IsVisibleFace(Block, EBlockDirection::SOUTH))
	{
		mesh.AppendTriangle(v5, v4, v7);
		mesh.AppendTriangle(v5, v7, v6);
	}
	// -y faces
	if (IsVisibleFace(Block, EBlockDirection::EAST))
	{
		mesh.AppendTriangle(v4, v1, v2);
		mesh.AppendTriangle(v4, v2, v7);
	}
	// +y faces
	if (IsVisibleFace(Block, EBlockDirection::WEST))
	{
		mesh.AppendTriangle(v0, v5, v6);
		mesh.AppendTriangle(v0, v6, v3);
	}
	// -z faces
	if (IsVisibleFace(Block, EBlockDirection::DOWN))
	{
		mesh.AppendTriangle(v5, v0, v1);
		mesh.AppendTriangle(v5, v1, v4);
	}
	// +z faces
	if (IsVisibleFace(Block, EBlockDirection::UP))
	{
		mesh.AppendTriangle(v2, v3, v6);
		mesh.AppendTriangle(v2, v6, v7);
	}
	//DynamicMeshComponent->SetDynamicMesh(DynamicMesh);
	DynamicMeshComponent->NotifyMeshUpdated();
	/*
	DynamicMeshComponent->ClearSimpleCollisionShapes(false);
	DynamicMeshComponent->SetSimpleCollisionShapes(DynamicMeshComponent->GetSimpleCollisionShapes(),true);*/
	//DynamicMeshComponent->UpdateCollision();
	UE_LOG(LogEnigmaVoxelChunk, Display, TEXT("Append new Block triangles -> %s"), *Block.Coordinates.ToString());
}


// Called every frame
void AChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
