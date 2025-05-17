// Fill out your copyright notice in the Description page of Project Settings.

#include "ChunkHolder.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "EnigmaVoxel/Core/Register/EnigmaRegistrationSubsystem.h"
#include "EnigmaVoxel/Core/World/EnigmaWorld.h"
#include "EnigmaVoxel/Modules/Block/Block.h"
#include "EnigmaVoxel/Modules/Block/Enum/BlockDirection.h"

FChunkHolder::FChunkHolder()
{
	int32 Size = Dimension.X * Dimension.Y * Dimension.Z;
	Blocks.SetNum(Size);
	MaterialToSection.Reserve(Size);
}

void FChunkHolder::RefreshMaterialCache()
{
	MaterialToSection.Reset();
	NextSectionIndex = 0;
}

int32 FChunkHolder::GetSectionIndexForMaterial(UMaterialInterface* InMaterial)
{
	if (!InMaterial)
	{
		UE_LOG(LogEnigmaVoxelChunk, Warning, TEXT("GetSectionIndexForMaterial nullptr"));
		return -1;
	}
	if (int32* Found = MaterialToSection.Find(InMaterial))
	{
		return *Found;
	}
	int32 NewIndex = ++NextSectionIndex;
	MaterialToSection.Add(InMaterial, NewIndex);
	return NewIndex;
}

int32 FChunkHolder::GetBlockIndex(const FIntVector& LocalCoords) const
{
	return LocalCoords.X + LocalCoords.Y * Dimension.X + LocalCoords.Z * Dimension.X * Dimension.Y;
}

FBlock& FChunkHolder::GetBlock(const FIntVector& LocalCoords)
{
	return Blocks[GetBlockIndex(LocalCoords)];
}

const FBlock& FChunkHolder::GetBlock(const FIntVector& LocalCoords) const
{
	return Blocks[GetBlockIndex(LocalCoords)];
}

void FChunkHolder::SetBlock(const FIntVector& LocalCoords, const FBlock& InBlockData)
{
	FBlock& Block = GetBlock(LocalCoords);
	Block         = InBlockData;
}


void FChunkHolder::SetBlock(const FIntVector& InCoords, FString Namespace, FString Path)
{
	UBlockDefinition* def = UEnigmaRegistrationSubsystem::BLOCK_GET_VALUE(Namespace, Path);
	SetBlock(InCoords, FBlock(InCoords, def, 100));
}

bool FChunkHolder::FillChunkWithArea(FIntVector Area, FString Namespace, FString Path)
{
	for (int z = 0; z < Area.Z; ++z)
	{
		for (int y = 0; y < Area.Y; ++y)
		{
			for (int x = 0; x < Area.X; ++x)
			{
				SetBlock(FIntVector(x, y, z), Namespace, Path);
			}
		}
	}
	return true;
}

void FChunkHolder::AddTicket()
{
	++RefCount;
	PendingUnloadUntil = 0.0;

	if (Stage == EChunkStage::PendingUnload || Stage == EChunkStage::Unloaded)
	{
		Stage = EChunkStage::Loading;
	}
}

void FChunkHolder::RemoveTicket(double Now, double Grace)
{
	if (RefCount == 0)
	{
		return;
	}
	check(RefCount>0)
	--RefCount;
	if (RefCount == 0)
	{
		Stage              = EChunkStage::PendingUnload;
		PendingUnloadUntil = Now + Grace;
	}
}

bool IsFaceVisibleInChunkData(const FChunkHolder& ChunkHolder, int x, int y, int z, EBlockDirection Direction)
{
	FIntVector3 blockPos(x, y, z);
	switch (Direction)
	{
	case EBlockDirection::NORTH:
		if (blockPos.X + 1 >= ChunkHolder.Dimension.X)
		{
			return true;
		}
		blockPos.X += 1;
		if (ChunkHolder.GetBlock(blockPos).Definition == nullptr)
		{
			return true;
		}
		return false;
	case EBlockDirection::SOUTH:
		if (blockPos.X - 1 < 0)
		{
			return true;
		}
		blockPos.X -= 1;
		if (ChunkHolder.GetBlock(blockPos).Definition == nullptr)
		{
			return true;
		}
		return false;
	case EBlockDirection::EAST:
		if (blockPos.Y + 1 >= ChunkHolder.Dimension.Y)
		{
			return true;
		}
		blockPos.Y += 1;
		if (ChunkHolder.GetBlock(blockPos).Definition == nullptr)
		{
			return true;
		}
		return false;
	case EBlockDirection::WEST:
		if (blockPos.Y - 1 < 0)
		{
			return true;
		}
		blockPos.Y -= 1;
		if (ChunkHolder.GetBlock(blockPos).Definition == nullptr)
		{
			return true;
		}
		return false;
	case EBlockDirection::UP:
		if (blockPos.Z + 1 >= ChunkHolder.Dimension.Z)
		{
			return true;
		}
		blockPos.Z += 1;
		if (ChunkHolder.GetBlock(blockPos).Definition == nullptr)
		{
			return true;
		}
		return false;
	case EBlockDirection::DOWN:
		if (blockPos.Z - 1 < 0)
		{
			return true;
		}
		blockPos.Z -= 1;
		if (ChunkHolder.GetBlock(blockPos).Definition == nullptr)
		{
			return true;
		}
		return false;
	}
	return false;
}

bool IsFaceVisible(UEnigmaWorld* World, const FChunkHolder& ChunkHolder, int x, int y, int z, EBlockDirection Direction)
{
	const FIntVector& Dim = ChunkHolder.Dimension;
	// ap the local coordinates (x, y, z) in the current block to the global block coordinates (block-based)
	auto GetGlobalBlockCoords = [&](int LocalX, int LocalY, int LocalZ)
	{
		int GlobalX = ChunkHolder.Coords.X * Dim.X + LocalX;
		int GlobalY = ChunkHolder.Coords.Y * Dim.Y + LocalY;
		int GlobalZ = ChunkHolder.Coords.Z * Dim.Z + LocalZ;
		return FIntVector(GlobalX, GlobalY, GlobalZ);
	};

	switch (Direction)
	{
	// +X
	case EBlockDirection::NORTH:
		{
			// Determine whether to cross the X range of this Chunk
			if (x + 1 >= Dim.X)
			{
				FIntVector        neighborGlobalPos = GetGlobalBlockCoords(x + 1, y, z);
				UBlockDefinition* neighborDef       = World->GetBlockAtBlockPos(neighborGlobalPos);
				// If the neighbor block is empty (or the neighbor block is not loaded => return nullptr) => visible
				return (neighborDef == nullptr);
			}
			// Inside this Chunk => directly look at the adjacent blocks
			if (ChunkHolder.GetBlock(FIntVector(x + 1, y, z)).Definition == nullptr)
			{
				return true; // air => visible
			}
			return false; // solid => invisible (need culled)
		}
	// break; // not strictly needed if we return

	// -X
	case EBlockDirection::SOUTH:
		{
			if (x - 1 < 0)
			{
				// 跨区块
				FIntVector        neighborGlobalPos = GetGlobalBlockCoords(x - 1, y, z);
				UBlockDefinition* neighborDef       = World->GetBlockAtBlockPos(neighborGlobalPos);
				return (neighborDef == nullptr);
			}
			// 本 Chunk
			if (ChunkHolder.GetBlock(FIntVector(x - 1, y, z)).Definition == nullptr)
			{
				return true;
			}
			return false;
		}

	// +Y
	case EBlockDirection::EAST:
		{
			if (y + 1 >= Dim.Y)
			{
				FIntVector        neighborGlobalPos = GetGlobalBlockCoords(x, y + 1, z);
				UBlockDefinition* neighborDef       = World->GetBlockAtBlockPos(neighborGlobalPos);
				return (neighborDef == nullptr);
			}
			if (ChunkHolder.GetBlock(FIntVector(x, y + 1, z)).Definition == nullptr)
			{
				return true;
			}
			return false;
		}

	// -Y
	case EBlockDirection::WEST:
		{
			if (y - 1 < 0)
			{
				FIntVector        neighborGlobalPos = GetGlobalBlockCoords(x, y - 1, z);
				UBlockDefinition* neighborDef       = World->GetBlockAtBlockPos(neighborGlobalPos);
				return (neighborDef == nullptr);
			}
			if (ChunkHolder.GetBlock(FIntVector(x, y - 1, z)).Definition == nullptr)
			{
				return true;
			}
			return false;
		}

	// +Z
	case EBlockDirection::UP:
		{
			if (z + 1 >= Dim.Z)
			{
				FIntVector        neighborGlobalPos = GetGlobalBlockCoords(x, y, z + 1);
				UBlockDefinition* neighborDef       = World->GetBlockAtBlockPos(neighborGlobalPos);
				return (neighborDef == nullptr);
			}
			if (ChunkHolder.GetBlock(FIntVector(x, y, z + 1)).Definition == nullptr)
			{
				return true;
			}
			return false;
		}

	// -Z
	case EBlockDirection::DOWN:
		{
			if (z - 1 < 0)
			{
				FIntVector        neighborGlobalPos = GetGlobalBlockCoords(x, y, z - 1);
				UBlockDefinition* neighborDef       = World->GetBlockAtBlockPos(neighborGlobalPos);
				return (neighborDef == nullptr);
			}
			if (ChunkHolder.GetBlock(FIntVector(x, y, z - 1)).Definition == nullptr)
			{
				return true;
			}
			return false;
		}
	default:
		return false;
	}
}

void AppendBoxForBlock(FDynamicMesh3& Mesh, const FBlock& Block, FChunkHolder& ChunkHolder)
{
	const FIntVector blockPos = Block.Coordinates;
	// Calculate the coordinates of each block in the world
	float   BlockSize = ChunkHolder.BlockSize;
	FVector blockMinPt(static_cast<float>(blockPos.X) * BlockSize, static_cast<float>(blockPos.Y) * BlockSize, static_cast<float>(blockPos.Z) * BlockSize);
	FVector blockMaxPt(blockMinPt.X + BlockSize, blockMinPt.Y + BlockSize, blockMinPt.Z + BlockSize);

	// Add 8 vertices
	int32 v0 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMinPt.Y, blockMinPt.Z));
	int32 v1 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMaxPt.Y, blockMinPt.Z));
	int32 v2 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMaxPt.Y, blockMaxPt.Z));
	int32 v3 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMinPt.Y, blockMaxPt.Z));
	int32 v4 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMaxPt.Y, blockMinPt.Z));
	int32 v5 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMinPt.Y, blockMinPt.Z));
	int32 v6 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMinPt.Y, blockMaxPt.Z));
	int32 v7 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMaxPt.Y, blockMaxPt.Z));

	// +X faces => EBlockDirection::NORTH
	if (IsFaceVisibleInChunkData(ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::NORTH))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::NORTH));
		Mesh.AppendTriangle(v1, v0, v3, sectionID);
		Mesh.AppendTriangle(v1, v3, v2, sectionID);
	}

	// -X faces => EBlockDirection::SOUTH
	if (IsFaceVisibleInChunkData(ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::SOUTH))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::SOUTH));
		Mesh.AppendTriangle(v5, v4, v7, sectionID);
		Mesh.AppendTriangle(v5, v7, v6, sectionID);
	}

	// +Y faces => EBlockDirection::EAST
	if (IsFaceVisibleInChunkData(ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::EAST))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::EAST));
		Mesh.AppendTriangle(v4, v1, v2, sectionID);
		Mesh.AppendTriangle(v4, v2, v7, sectionID);
	}

	// -Y faces => EBlockDirection::WEST
	if (IsFaceVisibleInChunkData(ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::WEST))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::WEST));
		Mesh.AppendTriangle(v0, v5, v6, sectionID);
		Mesh.AppendTriangle(v0, v6, v3, sectionID);
	}

	// -Z faces => EBlockDirection::DOWN
	if (IsFaceVisibleInChunkData(ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::DOWN))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::DOWN));
		Mesh.AppendTriangle(v5, v0, v1, sectionID);
		Mesh.AppendTriangle(v5, v1, v4, sectionID);
	}

	// +Z faces => EBlockDirection::UP
	if (IsFaceVisibleInChunkData(ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::UP))
	{
		UMaterialInterface* material  = Block.GetFacesMaterial(EBlockDirection::UP);
		int                 sectionID = ChunkHolder.GetSectionIndexForMaterial(material);
		Mesh.AppendTriangle(v2, v3, v6, sectionID);
		Mesh.AppendTriangle(v2, v6, v7, sectionID);
	}
}

void AppendBoxForBlock(UEnigmaWorld* World, FDynamicMesh3& Mesh, const FBlock& Block, FChunkHolder& ChunkHolder)
{
	const FIntVector blockPos = Block.Coordinates;
	// Calculate the diagonal point of the block in the world
	float   BlockSize = ChunkHolder.BlockSize;
	FVector blockMinPt(static_cast<float>(blockPos.X) * BlockSize, static_cast<float>(blockPos.Y) * BlockSize, static_cast<float>(blockPos.Z) * BlockSize);
	FVector blockMaxPt(blockMinPt.X + BlockSize, blockMinPt.Y + BlockSize, blockMinPt.Z + BlockSize);

	int32 v0 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMinPt.Y, blockMinPt.Z));
	int32 v1 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMaxPt.Y, blockMinPt.Z));
	int32 v2 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMaxPt.Y, blockMaxPt.Z));
	int32 v3 = Mesh.AppendVertex(FVector3d(blockMaxPt.X, blockMinPt.Y, blockMaxPt.Z));
	int32 v4 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMaxPt.Y, blockMinPt.Z));
	int32 v5 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMinPt.Y, blockMinPt.Z));
	int32 v6 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMinPt.Y, blockMaxPt.Z));
	int32 v7 = Mesh.AppendVertex(FVector3d(blockMinPt.X, blockMaxPt.Y, blockMaxPt.Z));

	// +X => EBlockDirection::NORTH
	if (IsFaceVisible(World, ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::NORTH))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::NORTH));
		Mesh.AppendTriangle(v1, v0, v3, sectionID);
		Mesh.AppendTriangle(v1, v3, v2, sectionID);
	}

	// -X => EBlockDirection::SOUTH
	if (IsFaceVisible(World, ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::SOUTH))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::SOUTH));
		Mesh.AppendTriangle(v5, v4, v7, sectionID);
		Mesh.AppendTriangle(v5, v7, v6, sectionID);
	}

	// +Y => EBlockDirection::EAST
	if (IsFaceVisible(World, ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::EAST))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::EAST));
		Mesh.AppendTriangle(v4, v1, v2, sectionID);
		Mesh.AppendTriangle(v4, v2, v7, sectionID);
	}

	// -Y => EBlockDirection::WEST
	if (IsFaceVisible(World, ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::WEST))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::WEST));
		Mesh.AppendTriangle(v0, v5, v6, sectionID);
		Mesh.AppendTriangle(v0, v6, v3, sectionID);
	}

	// -Z => EBlockDirection::DOWN
	if (IsFaceVisible(World, ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::DOWN))
	{
		int sectionID = ChunkHolder.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::DOWN));
		Mesh.AppendTriangle(v5, v0, v1, sectionID);
		Mesh.AppendTriangle(v5, v1, v4, sectionID);
	}

	// +Z => EBlockDirection::UP
	if (IsFaceVisible(World, ChunkHolder, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::UP))
	{
		UMaterialInterface* tempMaterial = Block.GetFacesMaterial(EBlockDirection::UP);
		int                 sectionID    = ChunkHolder.GetSectionIndexForMaterial(tempMaterial);
		Mesh.AppendTriangle(v2, v3, v6, sectionID);
		Mesh.AppendTriangle(v2, v6, v7, sectionID);
	}
}
