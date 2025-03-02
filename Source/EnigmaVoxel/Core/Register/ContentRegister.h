// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ContentRegister.generated.h"

class UDefinition;
/**
 * 
 */
UCLASS(Abstract)
class ENIGMAVOXEL_API UContentRegister : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString NameSpace = FString("Enigma");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UDefinition>> RegisterContext;

public:
	UFUNCTION(BlueprintCallable, Category="Register")
	virtual bool RegisterFromDataTable(UDataTable* DataTable);
	UFUNCTION(BlueprintCallable, Category="Register")
	virtual bool RegisterFromDef(TSubclassOf<UDefinition> definition);

	/// Query	
public:
	virtual bool         HasAlreadyRegistered(TObjectPtr<UDefinition> defInstance);
	virtual UDefinition* GetDefinitionByID(FString id);
};
