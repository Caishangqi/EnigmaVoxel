#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

UENUM(BlueprintType)
enum class EBlockDirection: uint8
{
	EAST = 0,
	WEST = 1,
	UP = 2,
	DOWN = 3,
	SOUTH = 4,
	NORTH = 5
};
