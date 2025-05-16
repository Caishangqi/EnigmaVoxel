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
	Unloaded,
	Loading,
	Ready,
	PendingUnload
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

	FChunkHolder(const FChunkHolder&)            = delete; // 禁拷贝
	FChunkHolder& operator=(const FChunkHolder&) = delete;
	FChunkHolder(FChunkHolder&&)                 = default; // 允移动
	FChunkHolder& operator=(FChunkHolder&&)      = default;

	/// Key
	FIntVector Coords = FIntVector::ZeroValue;

	/// Running State
	std::atomic<uint16>      RefCount{0};
	std::atomic<EChunkStage> Stage{EChunkStage::Unloaded};
	std::atomic<bool>        bDirty{false};
	std::atomic<bool>        bQueuedForRebuild{false};
	double                   PendingUnloadUntil = 0.0; // 0 == 未排队卸载

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
