// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaWorldSubsystem.h"

#include "EnigmaVoxel/EnigmaVoxel.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"

void UEnigmaWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogEnigmaVoxelWorld, Warning, TEXT("EnigmaWorldSubsystem::Initialize"));
	Super::Initialize(Collection);
}
