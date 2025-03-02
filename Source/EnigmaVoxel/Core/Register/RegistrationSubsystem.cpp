// Fill out your copyright notice in the Description page of Project Settings.


#include "RegistrationSubsystem.h"

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
