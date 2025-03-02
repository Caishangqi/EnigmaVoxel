// Fill out your copyright notice in the Description page of Project Settings.


#include "ContentRegister.h"

#include "Definition.h"

bool UContentRegister::RegisterFromDataTable(UDataTable* DataTable)
{
	return false;
}

bool UContentRegister::RegisterFromDef(TSubclassOf<UDefinition> definition)
{
	return false;
}

bool UContentRegister::HasAlreadyRegistered(TObjectPtr<UDefinition> defInstance)
{
	return false;
}

UDefinition* UContentRegister::GetDefinitionByID(FString id)
{
	for (TObjectPtr<UDefinition> Definition : RegisterContext)
	{
		if (Definition->ID == id)
		{
			return Definition;
		}
	}
	return NULL;
}
