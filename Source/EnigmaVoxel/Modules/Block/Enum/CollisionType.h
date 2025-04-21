#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

UENUM(BlueprintType)
enum class ECollisionType: uint8
{
	UNIT_BLOCK,
	UNIT_BLOCK_HALF,
	CUSTOM
};
