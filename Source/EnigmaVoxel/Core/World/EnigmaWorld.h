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
 * UEnigmaWorld 当成“逻辑和数据管理器”，在内部维护 Chunk 的数据结构，然后在需要显示或碰撞时，委托 UWorld 生成真正的 Actor。
 * 这样的设计也和 Minecraft 或 NeoForge Mod 十分类似：“世界数据”（你的 UEnigmaWorld） + “底层真实世界”（Unreal 的 UWorld）。
 *
 * 继续在 UEnigmaWorldSubsystem 中管理、实例化和查询你的 UEnigmaWorld，不必在编辑器里直接挂载它为关卡对象。只在游戏运行时中，
 * 去 SpawnActor<AChunk> 即可。
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

	static constexpr int32  ViewRadius  = 3; // 玩家视野半径(区块)
	static constexpr double GracePeriod = 1.0; // 卸载宽限

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
