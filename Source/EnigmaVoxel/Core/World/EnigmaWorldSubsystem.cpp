// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorldSubsystem.h"

#include "EnigmaWorld.h"
#include "EnigmaWorldDelegates.h"
#include "EnigmaVoxel/EnigmaVoxel.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"

void UEnigmaWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogEnigmaVoxelWorld, Display, TEXT("EnigmaWorldSubsystem::Initialize"));
	/// Development Only
	TObjectPtr<UWorld> currentUWorld = GetWorld();
	if (currentUWorld == nullptr)
	{
		UE_LOG(LogEnigmaVoxelWorld, Error, TEXT("Current world is NULL"));
	}
	else
	{
		// We let EnigmaWorld delegate the UWorld
		CreateEnigmaWorld(0)->SetUWorldTarget(currentUWorld);
	}

	// Time Handle
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UEnigmaWorldSubsystem::UpdateWorldTick, 0.05f, true);

	// Event Binding
	FEnigmaWorldDelegates::OnPlayerJoinWorld.AddUObject(this, &UEnigmaWorldSubsystem::OnPlayerJoinWorld);

	/// 
	Super::Initialize(Collection);
}

UEnigmaWorld* UEnigmaWorldSubsystem::CreateEnigmaWorld(int32 WorldID)
{
	UE_LOG(LogEnigmaVoxelWorld, Display, TEXT("Creating Enigma World, ID -> %d"), WorldID);
	UEnigmaWorld* result = NewObject<UEnigmaWorld>(this);
	LoadedWorlds.Add(WorldID, result);
	return result;
}

void UEnigmaWorldSubsystem::UpdateWorldTick()
{
	for (TTuple<int, TObjectPtr<UEnigmaWorld>> LoadedWorld : LoadedWorlds)
	{
		if (LoadedWorld.Value->GetEnableWorldTick())
		{
			LoadedWorld.Value->UpdateStreamingChunks();
		}
	}
}

void UEnigmaWorldSubsystem::OnPlayerJoinWorld(int32 WorldIndex, AActor* Player)
{
	if (LoadedWorlds[WorldIndex])
	{
		LoadedWorlds[WorldIndex]->AddEntity(Cast<APawn>(Player));
		UE_LOG(LogEnigmaVoxelWorld, Display, TEXT("Player %s Join the World %d"), *Player->GetName(), WorldIndex);
	}
}
