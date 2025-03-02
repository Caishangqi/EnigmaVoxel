// Fill out your copyright notice in the Description page of Project Settings.


#include "BlockRegister.h"

bool UBlockRegister::RegisterFromDataTable(UDataTable* DataTable)
{
	return Super::RegisterFromDataTable(DataTable);
}

bool UBlockRegister::RegisterFromDef(TSubclassOf<UDefinition> definition)
{
	return Super::RegisterFromDef(definition);
}
