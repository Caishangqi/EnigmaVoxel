#include "ChunkData.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "EnigmaVoxel/Core/Register/EnigmaRegistrationSubsystem.h"
#include "EnigmaVoxel/Core/World/EnigmaWorld.h"
#include "EnigmaVoxel/Modules/Block/Enum/BlockDirection.h"

FChunkData::FChunkData(): ChunkCoords(FIntVector::ZeroValue)
                          , ChunkDimension(FIntVector(16, 16, 16))
                          , BlockSize(100.f)
                          , ChunkState(EChunkLoadState::UNLOADED)
                          , bHasBuiltMesh(false)
{
	int32 Size = ChunkDimension.X * ChunkDimension.Y * ChunkDimension.Z;
	Blocks.SetNum(Size);
	MaterialToSectionMap.Reserve(Size);
}

int32 FChunkData::GetBlockIndex(const FIntVector& LocalCoords) const
{
	return LocalCoords.X
		+ LocalCoords.Y * ChunkDimension.X
		+ LocalCoords.Z * ChunkDimension.X * ChunkDimension.Y;
}

FBlock& FChunkData::GetBlock(const FIntVector& LocalCoords)
{
	return Blocks[GetBlockIndex(LocalCoords)];
}

const FBlock& FChunkData::GetBlock(const FIntVector& LocalCoords) const
{
	return Blocks[GetBlockIndex(LocalCoords)];
}

void FChunkData::SetBlock(const FIntVector& LocalCoords, const FBlock& InBlockData)
{
	FBlock& Block = GetBlock(LocalCoords);
	Block         = InBlockData;
}

void FChunkData::SetBlock(const FIntVector& InCoords, FString Namespace, FString Path)
{
	UBlockDefinition* def = UEnigmaRegistrationSubsystem::BLOCK_GET_VALUE(Namespace, Path);
	SetBlock(InCoords, FBlock(InCoords, def, 100));
}

bool FChunkData::FillChunkWithArea(FIntVector fillArea, FString Namespace, FString Path)
{
	for (int z = 0; z < fillArea.Z; z++)
	{
		for (int y = 0; y < fillArea.Y; y++)
		{
			for (int x = 0; x < fillArea.X; x++)
			{
				SetBlock(FIntVector(x, y, z), Namespace, Path);
			}
		}
	}
	return true;
}

bool FChunkData::RefreshMaterialData()
{
	MaterialToSectionMap.Empty();
	NextSectionIndex = 0;
	int32 Size       = ChunkDimension.X * ChunkDimension.Y * ChunkDimension.Z;
	MaterialToSectionMap.Reserve(Size);
	return true;
}


int32 FChunkData::GetSectionIndexForMaterial(UMaterialInterface* InMaterial)
{
	if (!InMaterial)
	{
		UE_LOG(LogEnigmaVoxelChunk, Warning, TEXT("GetSectionIndexForMaterial called with nullptr!"));
		return -1;
	}
	int32* FoundIndex = MaterialToSectionMap.Find(InMaterial);
	if (FoundIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundIndex = %d"), *FoundIndex)
		return *FoundIndex;
	}
	else
	{
		// 否则 => 创建新的 SectionIndex
		int32 NewSectionIndex = NextSectionIndex++;
		MaterialToSectionMap.Add(InMaterial, NewSectionIndex);
		return NewSectionIndex;
	}
}


/// return whether or not the block in the Chunk's facing is visible in chunk local space
/// @param ChunkData the ChunkData
/// @param x the block local x coordinate
/// @param y the block local y coordinate
/// @param z the block local z coordinate
/// @param Direction The 6 freedom direction
/// @return Whether or not the current Direction is visible for player
bool IsFaceVisibleInChunkData(const FChunkData& ChunkData, int x, int y, int z, EBlockDirection Direction)
{
	FIntVector3 blockPos(x, y, z);
	switch (Direction)
	{
	case EBlockDirection::NORTH:
		if (blockPos.X + 1 >= ChunkData.ChunkDimension.X)
			return true;
		blockPos.X += 1;
		if (ChunkData.GetBlock(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::SOUTH:
		if (blockPos.X - 1 < 0)
			return true;
		blockPos.X -= 1;
		if (ChunkData.GetBlock(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::EAST:
		if (blockPos.Y + 1 >= ChunkData.ChunkDimension.Y)
			return true;
		blockPos.Y += 1;
		if (ChunkData.GetBlock(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::WEST:
		if (blockPos.Y - 1 < 0)
			return true;
		blockPos.Y -= 1;
		if (ChunkData.GetBlock(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::UP:
		if (blockPos.Z + 1 >= ChunkData.ChunkDimension.Z)
			return true;
		blockPos.Z += 1;
		if (ChunkData.GetBlock(blockPos).Definition == nullptr)
			return true;
		return false;
	case EBlockDirection::DOWN:
		if (blockPos.Z - 1 < 0)
			return true;
		blockPos.Z -= 1;
		if (ChunkData.GetBlock(blockPos).Definition == nullptr)
			return true;
		return false;
	}
	return false;
}

/// return whether or not the block in the Chunk's facing is visible in world space
/// This method especially check the neighbourhood chunk edge cases, so it need an additional
/// UEnigmaWorld pointer.
/// @param World the UEnigmaWorld pointer that the chunk current in (and loaded)
/// @param ChunkData the ChunkData
/// @param x the block local x coordinate
/// @param y the block local y coordinate
/// @param z the block local z coordinate
/// @param Direction The 6 freedom direction
/// @return Whether or not the current Direction is visible for player
bool IsFaceVisible(UEnigmaWorld* World, const FChunkData& ChunkData, int x, int y, int z, EBlockDirection Direction)
{
	const FIntVector& Dim = ChunkData.ChunkDimension;
	// ap the local coordinates (x, y, z) in the current block to the global block coordinates (block-based)
	auto GetGlobalBlockCoords = [&](int LocalX, int LocalY, int LocalZ)
	{
		int GlobalX = ChunkData.ChunkCoords.X * Dim.X + LocalX;
		int GlobalY = ChunkData.ChunkCoords.Y * Dim.Y + LocalY;
		int GlobalZ = ChunkData.ChunkCoords.Z * Dim.Z + LocalZ;
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
			else
			{
				// Inside this Chunk => directly look at the adjacent blocks
				if (ChunkData.GetBlock(FIntVector(x + 1, y, z)).Definition == nullptr)
					return true; // air => visible
				return false; // solid => invisible (need culled)
			}
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
			else
			{
				// 本 Chunk
				if (ChunkData.GetBlock(FIntVector(x - 1, y, z)).Definition == nullptr)
					return true;
				return false;
			}
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
			else
			{
				if (ChunkData.GetBlock(FIntVector(x, y + 1, z)).Definition == nullptr)
					return true;
				return false;
			}
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
			else
			{
				if (ChunkData.GetBlock(FIntVector(x, y - 1, z)).Definition == nullptr)
					return true;
				return false;
			}
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
			else
			{
				if (ChunkData.GetBlock(FIntVector(x, y, z + 1)).Definition == nullptr)
					return true;
				return false;
			}
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
			else
			{
				if (ChunkData.GetBlock(FIntVector(x, y, z - 1)).Definition == nullptr)
					return true;
				return false;
			}
		}
	default:
		return false;
	}
}


/// Append Box to current Chunk dynamic mesh, this method would also
/// append additional Collision verts into Collision dynamic mesh.
/// The condition of collision will dependent on block definition properties
/// 
/// The method is not handle cross visibility between neighbour chunks, this
/// method only handle the visibility of chunk local space, and automatically treat
/// edge chunk blocks as visible if you need the check please use the overloaded method with World parameter
/// @param Mesh 
/// @param Block The Block slot of The chunk
/// @param ChunkData 
void AppendBoxForBlock(UE::Geometry::FDynamicMesh3& Mesh, const FBlock& Block, FChunkData& ChunkData)
{
	const FIntVector blockPos = Block.Coordinates;
	// Calculate the coordinates of each block in the world
	float   BlockSize = ChunkData.BlockSize;
	FVector blockMinPt(float(blockPos.X) * BlockSize, float(blockPos.Y) * BlockSize, float(blockPos.Z) * BlockSize);
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
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::NORTH))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::NORTH));
		Mesh.AppendTriangle(v1, v0, v3, sectionID);
		Mesh.AppendTriangle(v1, v3, v2, sectionID);
	}

	// -X faces => EBlockDirection::SOUTH
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::SOUTH))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::SOUTH));
		Mesh.AppendTriangle(v5, v4, v7, sectionID);
		Mesh.AppendTriangle(v5, v7, v6, sectionID);
	}

	// +Y faces => EBlockDirection::EAST
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::EAST))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::EAST));
		Mesh.AppendTriangle(v4, v1, v2, sectionID);
		Mesh.AppendTriangle(v4, v2, v7, sectionID);
	}

	// -Y faces => EBlockDirection::WEST
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::WEST))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::WEST));
		Mesh.AppendTriangle(v0, v5, v6, sectionID);
		Mesh.AppendTriangle(v0, v6, v3, sectionID);
	}

	// -Z faces => EBlockDirection::DOWN
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::DOWN))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::DOWN));
		Mesh.AppendTriangle(v5, v0, v1, sectionID);
		Mesh.AppendTriangle(v5, v1, v4, sectionID);
	}

	// +Z faces => EBlockDirection::UP
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::UP))
	{
		UMaterialInterface * material = Block.GetFacesMaterial(EBlockDirection::UP);
		int sectionID = ChunkData.GetSectionIndexForMaterial(material);
		Mesh.AppendTriangle(v2, v3, v6, sectionID);
		Mesh.AppendTriangle(v2, v6, v7, sectionID);
	}
}

/// Append Box to current Chunk dynamic mesh, this method would also
/// append additional Collision verts into Collision dynamic mesh.
/// The condition of collision will dependent on block definition properties
///
/// The additional World parameter is used to check the visibility of the block in the world space.
/// @param World 
/// @param Mesh 
/// @param Block 
/// @param ChunkData 
void AppendBoxForBlock(UEnigmaWorld* World, UE::Geometry::FDynamicMesh3& Mesh, const FBlock& Block, FChunkData& ChunkData)
{
	const FIntVector blockPos = Block.Coordinates;
	// Calculate the diagonal point of the block in the world
	float   BlockSize = ChunkData.BlockSize;
	FVector blockMinPt(float(blockPos.X) * BlockSize, float(blockPos.Y) * BlockSize, float(blockPos.Z) * BlockSize);
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
	if (IsFaceVisible(World, ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::NORTH))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::NORTH));
		Mesh.AppendTriangle(v1, v0, v3, sectionID);
		Mesh.AppendTriangle(v1, v3, v2, sectionID);
	}

	// -X => EBlockDirection::SOUTH
	if (IsFaceVisible(World, ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::SOUTH))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::SOUTH));
		Mesh.AppendTriangle(v5, v4, v7, sectionID);
		Mesh.AppendTriangle(v5, v7, v6, sectionID);
	}

	// +Y => EBlockDirection::EAST
	if (IsFaceVisible(World, ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::EAST))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::EAST));
		Mesh.AppendTriangle(v4, v1, v2, sectionID);
		Mesh.AppendTriangle(v4, v2, v7, sectionID);
	}

	// -Y => EBlockDirection::WEST
	if (IsFaceVisible(World, ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::WEST))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::WEST));
		Mesh.AppendTriangle(v0, v5, v6, sectionID);
		Mesh.AppendTriangle(v0, v6, v3, sectionID);
	}

	// -Z => EBlockDirection::DOWN
	if (IsFaceVisible(World, ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::DOWN))
	{
		int sectionID = ChunkData.GetSectionIndexForMaterial(Block.GetFacesMaterial(EBlockDirection::DOWN));
		Mesh.AppendTriangle(v5, v0, v1, sectionID);
		Mesh.AppendTriangle(v5, v1, v4, sectionID);
	}

	// +Z => EBlockDirection::UP
	if (IsFaceVisible(World, ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::UP))
	{
		UMaterialInterface* tempMaterial = Block.GetFacesMaterial(EBlockDirection::UP);
		int                 sectionID    = ChunkData.GetSectionIndexForMaterial(tempMaterial);
		Mesh.AppendTriangle(v2, v3, v6, sectionID);
		Mesh.AppendTriangle(v2, v6, v7, sectionID);
	}
}
