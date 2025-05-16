#include "WorldGen.hpp"

#include "EnigmaVoxel/Modules/Block/Block.h"
#include "EnigmaVoxel/Modules/Chunk/ChunkHolder.h"

void FWorldGen::GenerateFullChunk(FChunkHolder& H)
{
	//H.RefreshMaterialCache();
	H.FillChunkWithArea(FIntVector(16, 16, 8), "Enigma", "Blue Enigma Block");
	UE::Geometry::FDynamicMesh3 Tmp;
	for (int z = 0; z < H.Dimension.Z; ++z)
	{
		for (int y = 0; y < H.Dimension.Y; ++y)
		{
			for (int x = 0; x < H.Dimension.X; ++x)
			{
				const FBlock& B = H.GetBlock({x, y, z});
				if (!B.Definition)
				{
					continue;
				}
				AppendBoxForBlock(Tmp, B, H);
			}
		}
	}
	H.Mesh = MoveTemp(Tmp);
}

void FWorldGen::RebuildMesh(FChunkHolder& H)
{
	H.Mesh = UE::Geometry::FDynamicMesh3();
	UE::Geometry::FDynamicMesh3 Tmp;
	H.RefreshMaterialCache();
	for (int z = 0; z < H.Dimension.Z; ++z)
	{
		for (int y = 0; y < H.Dimension.Y; ++y)
		{
			for (int x = 0; x < H.Dimension.X; ++x)
			{
				const FBlock& B = H.GetBlock({x, y, z});
				if (!B.Definition)
				{
					continue;
				}
				AppendBoxForBlock(Tmp, B, H);
			}
		}
	}
	H.Mesh = MoveTemp(Tmp);
	H.bDirty.store(false);
}
