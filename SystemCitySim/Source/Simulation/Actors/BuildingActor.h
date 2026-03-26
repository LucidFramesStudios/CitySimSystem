#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/Components/SmartObjectComponent.h"
#include "BuildingActor.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API ABuildingActor : public AActor
{
    GENERATED_BODY()

public:
    ABuildingActor();
   
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
    USmartObjectComponent* SmartObjectComp;

    void SetupBuilding(FGameplayTag InType, int32 InMaxUsers);

    UFUNCTION()
    void OnTimeChanged(float NewTime);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMat;
};