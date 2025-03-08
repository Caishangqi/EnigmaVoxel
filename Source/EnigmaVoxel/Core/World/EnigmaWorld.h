// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnigmaVoxel/Modules/Chunk/Chunk.h"
#include "UObject/Object.h"
#include "EnigmaWorld.generated.h"

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
	virtual UWorld* GetWorld() const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Chunk")
	TMap<FIntVector, TObjectPtr<AChunk>> LoadedChunks;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Chunk")
	TArray<TObjectPtr<APawn>> Players;

public:
	UFUNCTION(BlueprintCallable, Category="World")
	bool SetUWorldTarget(UWorld* UnrealBuildInWorld);
	UFUNCTION(BlueprintCallable, Category="World")
	bool SetEnableWorldTick(bool Enable = true);
	UFUNCTION(BlueprintCallable, Category="World")
	bool GetEnableWorldTick();

	UEnigmaWorld();

	UFUNCTION(BlueprintCallable, Category="World")
	void UpdateStreamingChunks();

	/// Entity Management
	UFUNCTION(BlueprintCallable, Category="Entity Management")
	bool AddEntity(APawn* InEntity);
	UFUNCTION(BlueprintCallable, Category="Entity Management")
	bool RemoveEntity(APawn* InEntity);

	/// 

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="World")
	TObjectPtr<UWorld> CurrentUWorld = nullptr;

	bool EnableWorldTick = false;
};
