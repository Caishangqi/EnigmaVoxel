#include "Block.h"

#include "EnigmaVoxel/Core/Log/DefinedLog.h"

UMaterialInterface* FBlock::GetFacesMaterial(EBlockDirection Direction) const
{
	if (Definition == nullptr)
	{
		UE_LOG(LogEnigmaVoxelBlock, Error, TEXT("Block Definition is null!"))
		return nullptr; /// Consider return the missing texture
	}
	TObjectPtr<UStaticMesh> meshModel = Definition->BlockState.Variants.Find(BlockStateKey)->Model;
	return meshModel->GetMaterial(static_cast<int32>(Direction));
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
