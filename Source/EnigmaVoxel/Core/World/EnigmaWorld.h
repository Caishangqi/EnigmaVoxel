#pragma once

#include "CoreMinimal.h"
#include "Containers/Deque.h"
#include "EnigmaVoxel/Modules/Chunk/ChunkActor.h"
#include "UObject/Object.h"
#include "EnigmaWorld.generated.h"

enum class ETicketType : uint8;
struct FChunkHolder;
class UChunkWorkerPool;
/**
* UEnigmaWorld is used as a "logic and data manager" to maintain the data structure of Chunk internally, and then delegates UWorld to generate real Actor when display or collision is required.
* This design is also very similar to Minecraft or NeoForge Mod: "world data" (your UEnigmaWorld) + "underlying real world" (Unreal's UWorld).
*
* Continue to manage, instantiate and query your UEnigmaWorld in UEnigmaWorldSubsystem, without directly mounting it as a level object in the editor. Only when the game is running,
* Go to SpawnActor<AChunk>.
*/
UCLASS()
class ENIGMAVOXEL_API UEnigmaWorld : public UObject
{
	GENERATED_BODY()

public:
	UEnigmaWorld();
	virtual void BeginDestroy() override;

	virtual UWorld*                            GetWorld() const override;
	TSet<FIntVector>                           PrevVisibleSet;
	TMap<FIntVector, TUniquePtr<FChunkHolder>> Chunks;
	FCriticalSection                           ChunksMutex;

	void             GatherPlayerVisibleSet(TSet<FIntVector>& Out);
	void             FlushDirtyAndPending(double Now);
	void             Tick();
	TSet<FIntVector> GatherPlayerView(int radius = 3);
	void             ProcessTickets(const TSet<FIntVector>& Desired, double Now);
	void             PumpWorkerResults();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Chunk")
	TMap<FIntVector, TObjectPtr<AChunkActor>> LoadedChunks;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Chunk")
	TArray<TObjectPtr<APawn>> Players;

	static constexpr int32  ViewRadius  = 3; // Player's field of view radius (blocks)
	static constexpr double GracePeriod = 10; // Uninstall grace period

	UFUNCTION(BlueprintCallable, Category="World")
	bool SetUWorldTarget(UWorld* UnrealBuildInWorld);
	UFUNCTION(BlueprintCallable, Category="World")
	bool SetEnableWorldTick(bool Enable = true);
	UFUNCTION(BlueprintCallable, Category="World")
	bool GetEnableWorldTick();

	/// Query
	UFUNCTION(BlueprintCallable, Category="Query")
	static FIntVector WorldPosToChunkCoords(const FVector& WorldPos);
	UFUNCTION(BlueprintCallable, Category="Query")
	static FIntVector BlockPosToChunkCoords(const FIntVector& BlockPos);
	UFUNCTION(BlueprintCallable, Category="Query")
	static FIntVector WorldPosToChunkLocalCoords(const FVector& WorldPos);
	UFUNCTION(BlueprintCallable, Category="Query")
	static FIntVector BlockPosToChunkLocalCoords(const FIntVector& BlockPos);
	UFUNCTION(BlueprintCallable, Category="Query")
	UBlockDefinition* GetBlockAtWorldPos(const FVector& WorldPos);
	UFUNCTION(BlueprintCallable, Category="Query")
	UBlockDefinition* GetBlockAtBlockPos(const FIntVector& BlockPos);

	/// Notify
	void NotifyNeighborsChunkLoaded(FIntVector ChunkCoords);

	/// Chunk Streaming
	UFUNCTION(BlueprintCallable, Category="World")
	void UpdateStreamingChunks();
	/// Entity Management
	UFUNCTION(BlueprintCallable, Category="Entity Management")
	bool AddEntity(APawn* InEntity);
	UFUNCTION(BlueprintCallable, Category="Entity Management")
	bool RemoveEntity(APawn* InEntity);

	/// 

	/// Thread Management
	void InitializeChunkWorkerPool();
	void ShutdownChunkWorkerPool();

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="World")
	TObjectPtr<UWorld> CurrentUWorld = nullptr;

	bool EnableWorldTick = true;

private:
	/// Thread Pool and Workers
	UPROPERTY()
	TObjectPtr<UChunkWorkerPool> ChunkWorkerPool = nullptr;
};
