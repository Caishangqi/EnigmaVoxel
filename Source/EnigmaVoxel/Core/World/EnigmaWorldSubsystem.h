// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EnigmaWorldSubsystem.generated.h"

class UEnigmaWorld;
/**
 * 
 */
UCLASS()
class ENIGMAVOXEL_API UEnigmaWorldSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="World")
	TMap<int32, TObjectPtr<UEnigmaWorld>> LoadedWorlds;

private:
};
