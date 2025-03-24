#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
UENUM(BlueprintType)
enum class EBlockDirection: uint8
{
	NORTH,
	SOUTH,
	EAST,
	WEST,
	UP,
	DOWN
};