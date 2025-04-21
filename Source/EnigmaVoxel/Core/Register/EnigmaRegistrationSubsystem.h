// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ContentRegister.h"
#include "EnigmaVoxel/Modules/Block/BlockRegister.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EnigmaRegistrationSubsystem.generated.h"

class URegistrationDelegates;
class UContentRegister;
class UBlockDefinition;

USTRUCT(BlueprintType)
struct FResourceRegister
{
	GENERATED_BODY()

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
class ENIGMAVOXEL_API UEnigmaRegistrationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Registration")
	static UEnigmaRegistrationSubsystem* GetRegistrationSubsystem();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	TMap<FString, FResourceRegister> RegisterMap;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitProperties() override;

	UFUNCTION(BlueprintCallable, Category = "Registration")
	static UBlockRegister* BLOCK(FString NameSpace = "Enigma");

	/// Query
	UFUNCTION(BlueprintCallable, Category = "Registration")
	static UBlockDefinition* BLOCK_GET_VALUE(FString NameSpace = "Enigma", FString ID = "");

private:
	static UEnigmaRegistrationSubsystem* registrationSubsystem;
	static URegistrationDelegates*       EventDispatcher;
};
