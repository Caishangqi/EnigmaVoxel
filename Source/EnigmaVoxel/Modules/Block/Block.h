// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnigmaVoxel/Core/Register/Definition.h"
#include "UObject/Object.h"
#include "Block.generated.h"

/**
 * 
 */
UCLASS()
class ENIGMAVOXEL_API UBlock : public UDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block")
	TObjectPtr<UStaticMeshComponent> Model;

	
};
