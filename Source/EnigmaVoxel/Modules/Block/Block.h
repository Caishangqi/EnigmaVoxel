#pragma once

#include "CoreMinimal.h"
#include "BlockDefinition.h"
#include "Block.generated.h"

enum class EBlockDirection : uint8;

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

	// Which Variant key in block state should the current block use
	// For example, "" or "facing=north", etc.
	UPROPERTY()
	FString BlockStateKey = "";

	// Record the current status of the block
	UPROPERTY()
	int32 StateID = 0;

	UMaterialInterface* GetFacesMaterial(EBlockDirection Direction) const;

	FBlock();

	FBlock(const FIntVector& InCoords, UBlockDefinition* InDefinition, int32 InHealth = 100);
};
