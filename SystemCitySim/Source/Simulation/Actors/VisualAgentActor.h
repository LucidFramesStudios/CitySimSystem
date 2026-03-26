#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Simulation/AgentAIController.h"
#include "Simulation/Logging/SimulationLog.h"
#include "VisualAgentActor.generated.h"

class UCharacterMovementComponent;
class ACityGeneratorActor;

UCLASS()
class SIMULATIONSHOWCASE_API AVisualAgentActor : public ACharacter
{
    GENERATED_BODY()

public:

    AVisualAgentActor();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Simulation")
    int32 AssignedAgentID = -1;

    UPROPERTY()
    float SegmentProgress = 0.f;
    void DrawAgentDebug();
    UPROPERTY()
    bool bInitializedOnSegment = false;

    UPROPERTY(BlueprintReadOnly, Category = "Sim")
    FVector SimVelocity;
private:

    void SyncToSimulation(float DeltaTime);

    // =========================
    // SIDEWALK SYSTEM
    // =========================

    UPROPERTY()
    ACityGeneratorActor* CachedCity = nullptr;

    UPROPERTY()
    int32 CurrentSegmentIndex = -1;
};