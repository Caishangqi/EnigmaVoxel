// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "UObject/Object.h"
#include "ChunkHolder.generated.h"

enum class EBlockDirection : uint8;
class UEnigmaWorld;
struct FBlock;

UENUM()
enum class EChunkStage : uint8
{
	Unloaded, // No data in memory
	Loading, // Thread pool is generating full blocks + grids meshes
	Ready, // The Mesh has been constructed, but has not yet been copied into the Actor
	Loaded, // Mesh has been synchronized to Actor, and the block is active
	PendingUnload // Reference count = 0, waiting for the grace period to end before being destroyed
};

UENUM()
enum class ETicketType : uint8
{
	Player,
	Entity,
	Forced
};

/**
 * 
 */
struct FChunkHolder
{
	FChunkHolder();
	~FChunkHolder() = default;

	FChunkHolder(const FChunkHolder&)            = delete;
	FChunkHolder& operator=(const FChunkHolder&) = delete;
	FChunkHolder(FChunkHolder&&)                 = default;
	FChunkHolder& operator=(FChunkHolder&&)      = default;

	/// Key
	FIntVector Coords = FIntVector::ZeroValue;

	/// Running State
	std::atomic<uint16>      RefCount{0};
	std::atomic<EChunkStage> Stage{EChunkStage::Unloaded};
	std::atomic<bool>        bDirty{false};
	std::atomic<bool>        bNeedsNeighborNotify{false};
	std::atomic<bool>        bQueuedForRebuild{false};
	double                   PendingUnloadUntil = 0.0; // 0 == Not queued for unloading

	/// Data
	FIntVector                       Dimension{16, 16, 16};
	float                            BlockSize = 100.f;
	TArray<FBlock>                   Blocks;
	UE::Geometry::FDynamicMesh3      Mesh;
	TMap<UMaterialInterface*, int32> MaterialToSection;
	int32                            NextSectionIndex = 0;
	TSharedPtr<TFuture<void>>        BuildFuture;

	/// API
	void          RefreshMaterialCache();
	int32         GetSectionIndexForMaterial(UMaterialInterface*);
	int32         GetBlockIndex(const FIntVector& LocalCoords) const;
	FBlock&       GetBlock(const FIntVector& LocalCoords);
	const FBlock& GetBlock(const FIntVector& LocalCoords) const;
	void          SetBlock(const FIntVector& LocalCoords, const FBlock& InBlockData);
	void          SetBlock(const FIntVector& InCoords, FString Namespace = "Enigma", FString Path = "");

	bool FillChunkWithArea(FIntVector Area, FString Namespace = "Enigma", FString Path = "");

	// Ticket
	void AddTicket();
	void RemoveTicket(double Now, double Grace);
};

bool IsFaceVisibleInChunkData(const FChunkHolder& ChunkHolder, int x, int y, int z, EBlockDirection Direction);
bool IsFaceVisible(UEnigmaWorld* World, const FChunkHolder& ChunkHolder, int x, int y, int z, EBlockDirection Direction);
void AppendBoxForBlock(UE::Geometry::FDynamicMesh3& Mesh, const FBlock& Block, FChunkHolder& ChunkHolder);
void AppendBoxForBlock(UEnigmaWorld* World, UE::Geometry::FDynamicMesh3& Mesh, const FBlock& Block, FChunkHolder& ChunkHolder);
