#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/Subsystems/RoadGraphSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "NeedsSubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API UNeedsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void ProcessTick(float DeltaTime, ESimPhase Phase);


    TArray<FAgentData>& GetActiveAgents()
    {
        return ActiveAgents;
    }

    FAgentData* GetAgentData(int32 AgentID);
    int32 FindClosestEdgeToLocation(
        const FVector& Location,
        const TArray<FRoadEdge>& Edges) const;
    void ScoreAgentNeeds(FAgentData& Agent, float DeltaTime);

    void EvaluateAgentSchedule(FAgentData& Agent, float WorldTime);


private:

    UPROPERTY(EditAnywhere, Category = "Simulation|Agents", meta = (ClampMin = "1", ClampMax = "5000"))
    int32 InitialAgentCount = 300;
    int32 FindClosestSidewalkSegment(const FVector& Location, const TArray<struct FSidewalkSegment>& Segments) const;
    TArray<FAgentData> ActiveAgents;

    FRandomStream SimRandomStream;
};