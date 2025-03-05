// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EVGameInstance.generated.h"

class UGameInstanceEventDispatcher;
/**
 * 
 */
UCLASS(Blueprintable)
class ENIGMAVOXEL_API UEVGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void                   Init() override;
	virtual FGameInstancePIEResult InitializeForPlayInEditor(int32 PIEInstanceIndex, const FGameInstancePIEParameters& Params) override;

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnRegistrationInitialize();
	
private:
	static TObjectPtr<UEVGameInstance> GameInstance;

public:
	UFUNCTION(BlueprintCallable)
	static UEVGameInstance* GET_INSTANCE();
};
