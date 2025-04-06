#pragma once
#include "DynamicMesh/DynamicMesh3.h"
#include "EnigmaVoxel/Modules/Block/Block.h"
#include "ChunkData.generated.h"


class UEnigmaWorld;

enum class EChunkLoadState: uint8
{
	UNLOADED = 0,
	LOADING = 1,
	LOADED = 2,
	UNLOADING = 3
};

USTRUCT()
struct FChunkMeshData
{
	GENERATED_BODY()

public:
	UE::Geometry::FDynamicMesh3 Mesh;
};


USTRUCT()
struct FChunkData
{
	GENERATED_BODY()

public:
	// Chunk world coordinates (in Chunks)
	UPROPERTY()
	FIntVector ChunkCoords;

	UPROPERTY()
	FIntVector ChunkDimension;

	// All block data in each chunk
	UPROPERTY()
	TArray<FBlock> Blocks;

	// The actual size of each block
	UPROPERTY()
	float BlockSize;

	EChunkLoadState ChunkState;

	// Whether the rendering mesh has been constructed
	bool bHasBuiltMesh;

	// Constructed visible mesh data (can be passed directly to the game thread after the background thread has completed the generation)
	FChunkMeshData BuiltMeshData;

	// 用来记录：某个材质对应的 SectionIndex（在网格里即 GroupID）。
	// 当网格渲染时，如果多个三角形共用同一个 SectionIndex，就视为使用同一材质。
	UPROPERTY()
	TMap<UMaterialInterface*, int32> MaterialToSectionMap;

	// 存放实际要给组件设置的材质列表（对应 SectionIndex）
	UPROPERTY()
	TArray<UMaterialInterface*> CollectedMaterials;

	// 动态分配下一个可用的 Section 索引
	int32 NextSectionIndex = 0;

public:
	FChunkData();

	int32         GetBlockIndex(const FIntVector& LocalCoords) const;
	FBlock&       GetBlock(const FIntVector& LocalCoords);
	const FBlock& GetBlock(const FIntVector& LocalCoords) const;
	void          SetBlock(const FIntVector& LocalCoords, const FBlock& InBlockData);
	void          SetBlock(const FIntVector& InCoords, FString Namespace = "Enigma", FString Path = "");
	bool          FillChunkWithArea(FIntVector fillArea, FString Namespace = "Enigma", FString Path = "");
	bool          RefreshMaterialData();
	int32         GetSectionIndexForMaterial(UMaterialInterface* InMaterial);
};


struct FChunkInfo
{
	EChunkLoadState LoadState;
	FChunkData      ChunkData;
	bool            bIsDirty = false; // Need mesh rebuild

	FChunkInfo()
		: LoadState(EChunkLoadState::UNLOADED)
	{
	}
};

bool IsFaceVisibleInChunkData(const FChunkData& ChunkData, int x, int y, int z, EBlockDirection Direction);
bool IsFaceVisible(UEnigmaWorld* World, const FChunkData& ChunkData, int x, int y, int z, EBlockDirection Direction);
void AppendBoxForBlock(UE::Geometry::FDynamicMesh3& Mesh, const FBlock& Block, FChunkData& ChunkData);
void AppendBoxForBlock(UEnigmaWorld* World, UE::Geometry::FDynamicMesh3& Mesh, const FBlock& Block, FChunkData& ChunkData);
