// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnigmaVoxel/Modules/Block/Block.h"
#include "Runtime/GeometryFramework/Public/DynamicMeshActor.h"
#include "Chunk.generated.h"


UCLASS()
class ENIGMAVOXEL_API AChunk : public ADynamicMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AChunk();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/// Collision Dynamic Mesh
	/// Use for detach from visual because some block do not have collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UDynamicMeshComponent> CollisionDynamicMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block")
	FIntVector ChunkDimension = FIntVector(16, 16, 16);
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FBlock> Blocks;

protected:
	UFUNCTION(BlueprintCallable)
	int32 GetChunkBlockSize();

	// Query
	UFUNCTION(BlueprintCallable)
	FBlock& GetBlockAt(FIntVector InCoords);
	UFUNCTION(BlueprintCallable)
	int32 GetBlockIndexAt(FIntVector InCoords);
	UFUNCTION(BlueprintCallable)
	bool IsVisibleFace(FBlock& currentBlock, EBlockDirection InDirection);

public:
	// Setter
	/// Update Block by using FBlock Data, if the Block Definition inside
	/// FBlock is null the method will return false else return true. The
	/// null BlockDefinition will not be rendered
	/// @param InBlockData 
	/// @return whether or not the Definition is null
	UFUNCTION(BlueprintCallable)
	bool UpdateBlock(FBlock InBlockData);
	/// Update Block by using Block position, and resource location, this method
	/// would query the registration system for once and execute @seeUpdateBlock(FBlock InBlockData)
	/// It will return null if the Block Definition is null
	/// @param InCoords The block coords you want to replace
	/// @param Namespace The Namespace of the block Definition
	/// @param Path ThePath of the block Definition
	/// @return whether or not the Definition is null
	UFUNCTION(BlueprintCallable)
	bool UpdateBlockResourceLocation(FIntVector InCoords, FString Namespace = "Enigma", FString Path = "");

	/// Rebuild the whole chunk, it will read the Block Data Array from the beginning
	/// clear the current Dynamic Mesh also the Collision Dynamic Mesh
	/// @return whether or not rebuild successful
	UFUNCTION(BlueprintCallable)
	bool UpdateChunk();

	/// Utility Function Right now for quickly fill the chunk with blocks
	/// TODO: the functions inside chunk will move to world subsystem perhaps
public:
	UFUNCTION(BlueprintCallable)
	bool FillChunkWithXYZ(FIntVector fillArea, FString Namespace = "Enigma", FString Path = "");

private:
	/// Append Box to current Chunk dynamic mesh, this method would also
	/// append additional Collision verts into Collision dynamic mesh.
	/// The condition of collision will dependent on block definition properties
	/// @param Block The Block slot of The chunk
	void AppendBoxWithCollision(FBlock& Block);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
