#pragma once

#include "CoreMinimal.h"
#include "BlockDefinition.h"
#include "Block.generated.h"

// 一个简单的结构体，用于表示区块中单个方块实例
USTRUCT(BlueprintType)
struct FBlock
{
	GENERATED_BODY()

public:
	// 指向全局注册的方块定义
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UBlockDefinition> Definition = nullptr;

	// The block's coordinates in chunk
	UPROPERTY(BlueprintReadWrite)
	FIntVector Coordinates = FIntVector(0, 0, 0);

	// TODO: May Add and Maintain the Block World integer position

	// The current health of the block
	UPROPERTY(BlueprintReadWrite)
	int32 Health = 100;

	// Record the current status of the block
	UPROPERTY()
	int32 StateID = 0;

	FBlock()
		: Definition(nullptr)
		  , Coordinates(FIntVector::ZeroValue)
		  , Health(100) // 默认生命值
	{
	}

	FBlock(const FIntVector& InCoords, UBlockDefinition* InDefinition, int32 InHealth = 100)
		: Definition(InDefinition)
		  , Coordinates(InCoords)
		  , Health(InHealth)
	{
	}
};
