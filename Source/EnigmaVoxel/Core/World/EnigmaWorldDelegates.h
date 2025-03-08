#pragma once

#include "CoreMinimal.h"
#include "EnigmaWorld.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerJoinWorld, int32, AActor*);

struct ENIGMAVOXEL_API FEnigmaWorldDelegates
{
	FEnigmaWorldDelegates() = delete;
	
	static FOnPlayerJoinWorld OnPlayerJoinWorld;
};
