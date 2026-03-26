#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CityVisualManager.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API ACityVisualManager : public AActor
{
    GENERATED_BODY()

public:

    ACityVisualManager();

    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root;

    UPROPERTY()
    TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> HISMMap;

    void AddMeshInstance(UStaticMesh* Mesh, const FTransform& Transform);

    void ClearAll();
};