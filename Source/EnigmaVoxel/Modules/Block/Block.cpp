#include "Block.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"
#include "Enum/BlockDirection.h"

UMaterialInterface* FBlock::GetFacesMaterial(EBlockDirection Direction) const
{
	if (!Definition)
	{
		UE_LOG(LogEnigmaVoxelBlock, Error, TEXT("Block Definition is null!"));
		return nullptr;
	}

	FBlockVariantDefinition* Variant = Definition->BlockState.Variants.Find(BlockStateKey);
	if (!Variant)
	{
		Variant = Definition->BlockState.Variants.Find(TEXT("")); // fallback default key
		if (!Variant)
		{
			UE_LOG(LogEnigmaVoxelBlock, Warning, TEXT("Cannot find variant for key '%s' in block '%s'"), *BlockStateKey, *Definition->GetName());
			return nullptr;
		}
	}

	UStaticMesh* Mesh = Variant->Model.LoadSynchronous();
	if (!Mesh)
	{
		UE_LOG(LogEnigmaVoxelBlock, Error, TEXT("Block mesh is null for block '%s'"), *Definition->GetName());
		return nullptr;
	}

	return Mesh->GetMaterial(static_cast<uint8>(Direction));
}

FBlock::FBlock()
	: Definition(nullptr)
	  , Coordinates(FIntVector::ZeroValue)
	  , Health(100)
{
}

FBlock::FBlock(const FIntVector& InCoords, UBlockDefinition* InDefinition, int32 InHealth): Definition(InDefinition), Coordinates(InCoords), Health(InHealth)
{
}
