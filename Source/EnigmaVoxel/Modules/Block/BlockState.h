#pragma once
#include "CoreMinimal.h"
#include "BlockState.generated.h"

enum class EBlockDirection : uint8;

USTRUCT(BlueprintType)
struct FBlockState
{
	GENERATED_BODY()

public:
	// A reference to UStaticMesh for irregular meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> CustomMesh;

	// Or store texture/material information, such as the material of the six faces, UV offset, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EBlockDirection, UTexture2D*> FaceMaterials;
};
