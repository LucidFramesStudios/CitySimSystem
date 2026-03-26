#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PointLightComponent.h"
#include "StreetLightActor.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API AStreetLightActor : public AActor
{
    GENERATED_BODY()

public:
    AStreetLightActor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light")
    UPointLightComponent* LightComponent;

    UFUNCTION()
    void OnTimeChanged(float NewTime);
};