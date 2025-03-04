// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnigmaVoxel/Core/Register/Definition.h"
#include "UObject/Object.h"
#include "BlockDefinition.generated.h"


/**
 * 
 */
UENUM(BlueprintType)
enum class ECollisionType: uint8
{
	UNIT_BLOCK,
	UNIT_BLOCK_HALF,
	CUSTOM
};

UCLASS(Blueprintable, BlueprintType)
class ENIGMAVOXEL_API UBlockDefinition : public UDefinition
{
	GENERATED_BODY()

public:
	UBlockDefinition();

public:
	/// Internal Block ID, use for chunk storage
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Block Definition")
	int64 BlockID = 0;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Block Definition")
	ECollisionType CollisionType = ECollisionType::UNIT_BLOCK;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Definition")
	TObjectPtr<UStaticMeshComponent> Model;

public:
	/// We like Minecraft so we use their method
	/// Destroy logic could look Neo forge implementation quite perfect

	/// OnFinishDestroy
	/// OnBeginDestroy
	/// OnDrop
	/// OnDuringDestroy
	/// OnClick
	/// OnInteract
};
