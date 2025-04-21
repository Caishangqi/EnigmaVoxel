// Fill out your copyright notice in the Description page of Project Settings.


#include "BlockRegister.h"
#include "BlockDefinition.h"
#include "EnigmaVoxel/EnigmaVoxel.h"

bool UBlockRegister::RegisterFromDataTable(UDataTable* DataTable)
{
	return Super::RegisterFromDataTable(DataTable);
}

bool UBlockRegister::RegisterFromDef(TSubclassOf<UDefinition> definition)
{
	UBlockDefinition* block = NewObject<UBlockDefinition>(this, definition);
	if (block == nullptr)
	{
		UE_LOG(LogEnigmaVoxel, Error, TEXT("Failed to create UBlockDefinition definition -> %s"), *definition->GetName());
		return false;
	}
	if (HasAlreadyRegistered(block))
	{
		UE_LOG(LogEnigmaVoxel, Warning, TEXT("You had register duplicated ID object -> %s"), *block->ID);
	}
	RegisterContext.Add(block);
	UE_LOG(LogEnigmaVoxel, Log, TEXT("Registered Block object -> %s"), *block->ID);
	return true;
}

UDefinition* UBlockRegister::GetDefinitionByID(FString id)
{
	return Super::GetDefinitionByID(id);
}

bool UBlockRegister::HasAlreadyRegistered(TObjectPtr<UDefinition> defInstance)
{
	for (TObjectPtr<UDefinition> Definition : RegisterContext)
	{
		if (Definition->ID == defInstance->ID)
		{
			return true;
		}
	}
	return false;
}
