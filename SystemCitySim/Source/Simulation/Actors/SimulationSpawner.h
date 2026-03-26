#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimulationSpawner.generated.h"

class AVisualAgentActor;



UCLASS()
class SIMULATIONSHOWCASE_API ASimulationSpawner : public AActor
{
    GENERATED_BODY()

public:

    FTimerHandle SpawnTimerHandle;

    UFUNCTION()
    void SpawnAgentsSafe();
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AVisualAgentActor> VisualAgentClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class AVisualVehicleActor> VisualVehicleClass;

    UPROPERTY(EditAnywhere)
    int32 AgentCount = 10;

    void SpawnVehicles(class UVehicleSimulationSubsystem* VehSub);
};