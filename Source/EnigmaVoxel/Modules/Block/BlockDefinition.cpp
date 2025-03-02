// Fill out your copyright notice in the Description page of Project Settings.


#include "BlockDefinition.h"

UBlockDefinition::UBlockDefinition()
{
	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
}
