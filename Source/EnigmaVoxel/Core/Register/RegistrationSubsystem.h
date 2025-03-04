// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ContentRegister.h"
#include "EnigmaVoxel/Modules/Block/BlockRegister.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RegistrationSubsystem.generated.h"

class UContentRegister;
class UBlockDefinition;

USTRUCT(BlueprintType)
struct FResourceRegister
{
	GENERATED_BODY()

public:
	/// Block , Block Register
	/// Item , Item Register
	TMap<FString, TObjectPtr<UContentRegister>> ResourceTypeRegister;

	FResourceRegister()
	{
		ResourceTypeRegister.Add("Block", NewObject<UBlockRegister>());
	}
};

/**
 * 
 */
UCLASS(Blueprintable)
class ENIGMAVOXEL_API URegistrationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Registration")
	static URegistrationSubsystem* GetRegistrationSubsystem();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	TMap<FString, FResourceRegister> RegisterMap;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Registration")
	static UBlockRegister* BLOCK(FString NameSpace = "Enigma");

	/// Query
public:
	UFUNCTION(BlueprintCallable, Category = "Registration")
	static UBlockDefinition* BLOCK_GET_VALUE(FString NameSpace = "Enigma", FString ID = "");

private:
	static URegistrationSubsystem* registrationSubsystem;
};
