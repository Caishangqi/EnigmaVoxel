// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockState.h"
#include "EnigmaVoxel/Core/Register/Definition.h"
#include "Enum/CollisionType.h"
#include "UObject/Object.h"
#include "BlockDefinition.generated.h"


/**
 * 
 */

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

	/// TODO: use block properties builder in the future
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Block Definition")
	ECollisionType CollisionType = ECollisionType::UNIT_BLOCK;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Block Definition")
	bool bIsOpaque = false;

	/// Deprecated: Use BlockState instead
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Definition")
	TObjectPtr<UStaticMeshComponent> Model;

	///If you want to distinguish multiple block states files like Minecraft (such as multiple "subtypes"), you can also change
	///it to TArray<FBlockState> or TMap<FString, FBlockState>; but most of the time "one block → one block state + multiple
	///variants" is enough.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block Definition")
	FBlockState BlockState;

	// Indicates which StateID this block uses by default when placed/loaded
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block Definition")
	int32 DefaultStateID = 0;

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
