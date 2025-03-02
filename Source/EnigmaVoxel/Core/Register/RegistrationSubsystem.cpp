// Fill out your copyright notice in the Description page of Project Settings.


#include "RegistrationSubsystem.h"

#include "EnigmaVoxel/EnigmaVoxel.h"
#include "EnigmaVoxel/Modules/Block/BlockDefinition.h"
#include "EnigmaVoxel/Modules/Block/BlockRegister.h"

URegistrationSubsystem* URegistrationSubsystem::registrationSubsystem = nullptr;

URegistrationSubsystem* URegistrationSubsystem::GetRegistrationSubsystem()
{
	return registrationSubsystem;
}

void URegistrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	registrationSubsystem = this;
}

UBlockRegister* URegistrationSubsystem::BLOCK(FString NameSpace)
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

UBlockDefinition* URegistrationSubsystem::BLOCK_GET_VALUE(FString NameSpace, FString ID)
{
	UBlockRegister* blockRegister = BLOCK(NameSpace);
	return Cast<UBlockDefinition>(blockRegister->GetDefinitionByID(ID));
}
