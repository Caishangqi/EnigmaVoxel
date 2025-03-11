#include "ChunkData.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "EnigmaVoxel/Core/Register/EnigmaRegistrationSubsystem.h"

FChunkData::FChunkData(): ChunkCoords(FIntVector::ZeroValue)
                          , ChunkDimension(FIntVector(16, 16, 16))
                          , BlockSize(100.f)
                          , ChunkState(EChunkLoadState::UNLOADED)
                          , bHasBuiltMesh(false)
{
	int32 Size = ChunkDimension.X * ChunkDimension.Y * ChunkDimension.Z;
	Blocks.SetNum(Size);
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

void AppendBoxForBlock(UE::Geometry::FDynamicMesh3& Mesh, const FBlock& Block, const FChunkData& ChunkData)
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
		Mesh.AppendTriangle(v1, v0, v3);
		Mesh.AppendTriangle(v1, v3, v2);
	}

	// -X faces => EBlockDirection::SOUTH
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::SOUTH))
	{
		Mesh.AppendTriangle(v5, v4, v7);
		Mesh.AppendTriangle(v5, v7, v6);
	}

	// +Y faces => EBlockDirection::EAST
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::EAST))
	{
		Mesh.AppendTriangle(v4, v1, v2);
		Mesh.AppendTriangle(v4, v2, v7);
	}

	// -Y faces => EBlockDirection::WEST
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::WEST))
	{
		Mesh.AppendTriangle(v0, v5, v6);
		Mesh.AppendTriangle(v0, v6, v3);
	}

	// -Z faces => EBlockDirection::DOWN
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::DOWN))
	{
		Mesh.AppendTriangle(v5, v0, v1);
		Mesh.AppendTriangle(v5, v1, v4);
	}

	// +Z faces => EBlockDirection::UP
	if (IsFaceVisibleInChunkData(ChunkData, blockPos.X, blockPos.Y, blockPos.Z, EBlockDirection::UP))
	{
		Mesh.AppendTriangle(v2, v3, v6);
		Mesh.AppendTriangle(v2, v6, v7);
	}
}
