// Fill out your copyright notice in the Description page of Project Settings.


#include "EVGameInstance.h"

#include "Register/RegistrationDelegates.h"

TObjectPtr<UEVGameInstance> UEVGameInstance::GameInstance = nullptr;

void UEVGameInstance::Init()
{
	GameInstance = this;
	UE_LOG(LogTemp, Log, TEXT("UEVGameInstance initialized"));
	FRegistrationDelegates::OnRegistrationInitialize.AddLambda([this]()
	{
		GameInstance->OnRegistrationInitialize();
	});
	Super::Init();
}

FGameInstancePIEResult UEVGameInstance::InitializeForPlayInEditor(int32 PIEInstanceIndex, const FGameInstancePIEParameters& Params)
{
	return Super::InitializeForPlayInEditor(PIEInstanceIndex, Params);
}

void UEVGameInstance::OnRegistrationInitialize_Implementation()
{
}


UEVGameInstance* UEVGameInstance::GET_INSTANCE()
{
	return GameInstance;
}
