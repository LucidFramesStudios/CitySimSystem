#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "SimAISubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API USimAISubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void ProcessTick(ESimPhase Phase, float WorldTime);

    void EvaluateCommitment(FAgentData& Agent, float DeltaTime);
    void MakeDecision(FAgentData& Agent, float WorldTime);
    void ExecuteMovement(FAgentData& Agent, float DeltaTime, const TArray<FAgentData>& AllAgents);

private:
    TArray<struct FSmartObjectEntry*> FindBestSmartObjects(const FAgentData& Agent, FGameplayTag ObjectType);
};