#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingRecipe.generated.h"

USTRUCT(BlueprintType)
struct FBuildingRecipe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    UStaticMesh* BaseMesh;

    UPROPERTY(EditAnywhere)
    TArray<UStaticMesh*> BodyMeshes;

    UPROPERTY(EditAnywhere)
    UStaticMesh* RoofMesh;

    UPROPERTY(EditAnywhere)
    float FloorHeight = 400.f;

    UPROPERTY(EditAnywhere)
    float TaperFactor = 0.97f;
};