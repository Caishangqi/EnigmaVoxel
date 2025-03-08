// Fill out your copyright notice in the Description page of Project Settings.


#include "DefaultDebugPlayer.h"

#include "EnigmaVoxel/Core/World/EnigmaWorldDelegates.h"


// Sets default values
ADefaultDebugPlayer::ADefaultDebugPlayer()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ADefaultDebugPlayer::BeginPlay()
{
	FEnigmaWorldDelegates::OnPlayerJoinWorld.Broadcast(0,Cast<AActor>(this));
	Super::BeginPlay();
	
}

// Called every frame
void ADefaultDebugPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ADefaultDebugPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

