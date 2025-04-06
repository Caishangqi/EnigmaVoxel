#pragma once
#include "CoreMinimal.h"
#include "BlockState.generated.h"

USTRUCT(BlueprintType)
struct FBlockVariantDefinition
{
	GENERATED_BODY()

public:
	// This is where the reference to the Mesh is stored. It can be a direct UStaticMesh* or a TSoftObjectPtr<UStaticMesh>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block State")
	TObjectPtr<UStaticMesh> Model;

	// If you have other information, such as material, collision, UV information, you can put it here:
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block State")
	// UMaterialInterface* OverrideMaterial = nullptr;

	// ...etc
};

USTRUCT(BlueprintType)
struct FBlockState
{
	GENERATED_BODY()

public:
	// In Minecraft JSON: "variants": { "facing=north": {...}, "facing=south": {...} }
	// Use FString as key here, can also use FName
	// The value is the FBlockVariantDefinition you defined above, including Mesh/material, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block State")
	TMap<FString, FBlockVariantDefinition> Variants;

	// If you want to simulate the default value (key = "") in Minecraft, you can directly add a Key="", in the editor
	// and give the corresponding Model.

	// If there are more common properties, you can also add them here:
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block State")
	// bool bIsOpaque = true;
};
