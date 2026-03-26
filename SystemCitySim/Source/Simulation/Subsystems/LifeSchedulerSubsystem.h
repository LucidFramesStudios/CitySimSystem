#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LifeSchedulerSubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API ULifeSchedulerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void ProcessTick(float WorldTime);
    void EvaluateAgentSchedule(struct FAgentData& Agent, float WorldTime);

private:
    
    float GetTimeDifference(float StartTime, float EndTime) const;
};