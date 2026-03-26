#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "SocialBehaviorSubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API USocialBehaviorSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void ProcessTick(float WorldTime);

private:
    void HandleSocialDecisions(FAgentData& Agent, float WorldTime, class UNeedsSubsystem* NeedsSub, class UInteractionSubsystem* IntSub, class UCityEventSubsystem* EventSub);
};