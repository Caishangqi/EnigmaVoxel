// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorld.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "GameFramework/DefaultPawn.h"
#include "Kismet/GameplayStatics.h"

UWorld* UEnigmaWorld::GetWorld() const
{
	return CurrentUWorld;
}

bool UEnigmaWorld::SetUWorldTarget(UWorld* UnrealBuildInWorld)
{
	CurrentUWorld = UnrealBuildInWorld;
	if (CurrentUWorld == nullptr)
		return false;
	return true;
}

bool UEnigmaWorld::SetEnableWorldTick(bool Enable)
{
	EnableWorldTick = Enable;
	if (EnableWorldTick)
		return true;
	return false;
}

bool UEnigmaWorld::GetEnableWorldTick()
{
	return EnableWorldTick;
}

UEnigmaWorld::UEnigmaWorld()
{
}

void UEnigmaWorld::UpdateStreamingChunks()
{
}

bool UEnigmaWorld::AddEntity(APawn* InEntity)
{
	if (InEntity)
	{
		Players.Add(InEntity);
		return true;
	}
	return false;
}

bool UEnigmaWorld::RemoveEntity(APawn* InEntity)
{
	int numOfRemove = Players.Remove(InEntity);
	if (numOfRemove == 0)
	{
		return false;
	}
	return true;
}
