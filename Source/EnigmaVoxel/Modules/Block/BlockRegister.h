// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnigmaVoxel/Core/Register/ContentRegister.h"
#include "UObject/Object.h"
#include "BlockRegister.generated.h"

/**
 * 
 */
UCLASS()
class ENIGMAVOXEL_API UBlockRegister : public UContentRegister
{
	GENERATED_BODY()

public:
	virtual bool RegisterFromDataTable(UDataTable* DataTable) override;
	virtual bool RegisterFromDef(TSubclassOf<UDefinition> definition) override;
};
