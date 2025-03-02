#pragma once

#include "CoreMinimal.h"
#include "BlockDefinition.h"

// 一个简单的结构体，用于表示区块中单个方块实例
struct FBlock
{
	// 指向全局注册的方块定义
	UBlockDefinition* Definition = nullptr;

	// 方块在世界或区块中的坐标（例如使用整数坐标）
	FIntVector Coordinates;

	// 例如方块的当前健康值
	int32 Health = 100;

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
