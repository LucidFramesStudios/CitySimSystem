#include "Simulation/Subsystems/SocialBehaviorSubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Subsystems/CityEventSubsystem.h"
#include "Simulation/Subsystems/SimulationRandomSubsystem.h"
#include "Simulation/SimulationTags.h"

void USocialBehaviorSubsystem::ProcessTick(float WorldTime)
{
    return;
}


void USocialBehaviorSubsystem::HandleSocialDecisions(
    FAgentData& Agent,
    float WorldTime,
    UNeedsSubsystem* NeedsSub,
    UInteractionSubsystem* IntSub,
    UCityEventSubsystem* EventSub)
{
    return;
}