// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnigmaVoxel/Core/Register/Definition.h"
#include "UObject/Object.h"
#include "BlockDefinition.generated.h"

/**
 * 
 */
UCLASS(Blueprintable,BlueprintType)
class ENIGMAVOXEL_API UBlockDefinition : public UDefinition
{
	GENERATED_BODY()

public:
	UBlockDefinition();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block")
	TObjectPtr<UStaticMeshComponent> Model;
};
