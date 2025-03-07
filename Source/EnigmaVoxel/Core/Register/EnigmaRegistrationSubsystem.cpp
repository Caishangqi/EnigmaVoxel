// Fill out your copyright notice in the Description page of Project Settings.


#include "EnigmaRegistrationSubsystem.h"

#include "RegistrationDelegates.h"
#include "EnigmaVoxel/EnigmaVoxel.h"
#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "EnigmaVoxel/Modules/Block/BlockDefinition.h"
#include "EnigmaVoxel/Modules/Block/BlockRegister.h"

UEnigmaRegistrationSubsystem* UEnigmaRegistrationSubsystem::registrationSubsystem = nullptr;
URegistrationDelegates*       UEnigmaRegistrationSubsystem::EventDispatcher       = nullptr;

UEnigmaRegistrationSubsystem* UEnigmaRegistrationSubsystem::GetRegistrationSubsystem()
{
	return registrationSubsystem;
}


void UEnigmaRegistrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	registrationSubsystem = this;
	FRegistrationDelegates::OnRegistrationInitialize.Broadcast();
	UE_LOG(LogEnigmaVoxelRegister, Log, TEXT("EnigmaRegistrationSubsystem::Initialize"));
	Super::Initialize(Collection);
}

void UEnigmaRegistrationSubsystem::PostInitProperties()
{
	Super::PostInitProperties();
}

UBlockRegister* UEnigmaRegistrationSubsystem::BLOCK(FString NameSpace)
{
	TArray<FString> Namespaces;
	registrationSubsystem->RegisterMap.GetKeys(Namespaces);
	if (!Namespaces.Contains(NameSpace))
	{
		registrationSubsystem->RegisterMap.Add(NameSpace, FResourceRegister());
		UE_LOG(LogEnigmaVoxel, Log, TEXT("Register new Namespace: %s"), *NameSpace);
	}
	FResourceRegister& registers = registrationSubsystem->RegisterMap[NameSpace];
	if (registers.ResourceTypeRegister.Contains("Block"))
	{
		return Cast<UBlockRegister>(registers.ResourceTypeRegister["Block"]);
	}
	else
	{
		registers.ResourceTypeRegister.Add("Block", NewObject<UBlockRegister>());
		return Cast<UBlockRegister>(registers.ResourceTypeRegister["Block"]);
	}
}

UBlockDefinition* UEnigmaRegistrationSubsystem::BLOCK_GET_VALUE(FString NameSpace, FString ID)
{
	UBlockRegister* blockRegister = BLOCK(NameSpace);
	return Cast<UBlockDefinition>(blockRegister->GetDefinitionByID(ID));
}
