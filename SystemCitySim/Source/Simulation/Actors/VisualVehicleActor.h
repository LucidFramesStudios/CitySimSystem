#pragma once

#include "CoreMinimal.h"
#include "Simulation/Debug/DebugHUD.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "VisualVehicleActor.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API AVisualVehicleActor : public AActor
{
    GENERATED_BODY()

public:
    AVisualVehicleActor();
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Simulation")
    int32 AssignedVehicleID = -1;
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;
    void DrawVehicleDebug();
    
private:
    void SyncToSimulation(float DeltaTime);
};