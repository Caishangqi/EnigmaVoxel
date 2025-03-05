// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

DECLARE_MULTICAST_DELEGATE(FOnRegistrationInitialize);

struct ENIGMAVOXEL_API FRegistrationDelegates
{
	FRegistrationDelegates() = delete;
	static FOnRegistrationInitialize OnRegistrationInitialize;

	// static TMulticastDelegate<void(FString)> OnBlockRegistered;
};
