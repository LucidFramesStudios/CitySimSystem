#include "Simulation/Subsystems/LifeSchedulerSubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/SimulationTags.h"

void ULifeSchedulerSubsystem::ProcessTick(float WorldTime)
{
    UNeedsSubsystem* NeedsSub = GetWorld()->GetSubsystem<UNeedsSubsystem>();
    if (!NeedsSub) return;

    for (FAgentData& Agent : NeedsSub->GetActiveAgents())
    {
        
        
        EvaluateAgentSchedule(Agent, WorldTime);
    }
}

void ULifeSchedulerSubsystem::EvaluateAgentSchedule(FAgentData& Agent, float WorldTime)
{
    ELifePhase NextPhase = Agent.CurrentPhase;
    float CommuteStagger = (Agent.AgentID % 60) * (1.0f / 60.0f);
    float StartCommuteTime = 8.0f + CommuteStagger;
    float StartWorkTime = 9.0f + CommuteStagger;

    if (WorldTime >= 6.0f && WorldTime < StartCommuteTime) NextPhase = ELifePhase::WakeUp;
    else if (WorldTime >= StartCommuteTime && WorldTime < StartWorkTime) NextPhase = ELifePhase::CommuteToWork;
    else if (WorldTime >= StartWorkTime && WorldTime < 13.0f) NextPhase = ELifePhase::Work;
    else if (WorldTime >= 13.0f && WorldTime < 14.0f) NextPhase = ELifePhase::CoffeeBreak;
    else if (WorldTime >= 14.0f && WorldTime < 18.0f) NextPhase = ELifePhase::Work;
    else if (WorldTime >= 18.0f && WorldTime < 19.0f) NextPhase = ELifePhase::CommuteHome;
    else if (WorldTime >= 19.0f && WorldTime < 22.0f) NextPhase = ELifePhase::Relax;
    else NextPhase = ELifePhase::Sleep;

    if (Agent.CurrentPhase != NextPhase)
    {
        
        FGameplayTag NextGoal = FSimTags::Type_Home;
        switch (NextPhase)
        {
        case ELifePhase::Work:
        case ELifePhase::CommuteToWork: NextGoal = FSimTags::Type_Office; break;
        case ELifePhase::Sleep:
        case ELifePhase::CommuteHome: NextGoal = FSimTags::Type_Home; break;
        case ELifePhase::CoffeeBreak: NextGoal = FSimTags::Type_Cafe; break;
        case ELifePhase::Relax:
        case ELifePhase::Roam: NextGoal = (Agent.AgentID % 2 == 0) ? FSimTags::Type_Park : FSimTags::Type_Shop; break;
        default: NextGoal = FSimTags::Type_Home; break; 
        }

        
        bool bKeepCurrentState = (Agent.ActiveGoal == NextGoal);

        Agent.CurrentPhase = NextPhase;
        Agent.PhaseStartTime = WorldTime;
        Agent.FailedTargets.Empty();

        
        if (!bKeepCurrentState)
        {
            
            Agent.bIsCommittedToGoal = false;
            Agent.State = EAgentState::Decision;

            
            if (Agent.bInsideBuilding || Agent.ActiveInteractableID != -1)
            {
                if (UInteractionSubsystem* IntSub = GetWorld()->GetSubsystem<UInteractionSubsystem>())
                {
                    IntSub->ReleaseSlot(Agent.ActiveInteractableID, Agent.AgentID);
                }
                Agent.bInsideBuilding = false;
            }

            Agent.ActiveInteractableID = -1;
            Agent.CurrentPath.Empty();
            Agent.SimVelocity = FVector::ZeroVector;
        }
    }

    switch (Agent.CurrentPhase)
    {
    case ELifePhase::Work:
    case ELifePhase::CommuteToWork:
        Agent.ActiveGoal = FSimTags::Type_Office;
        break;
    case ELifePhase::Sleep:
    case ELifePhase::CommuteHome:
        Agent.ActiveGoal = FSimTags::Type_Home; 
        
        
        break;  
    case ELifePhase::CoffeeBreak:
        Agent.ActiveGoal = FSimTags::Type_Cafe;
        break;
    case ELifePhase::Relax:
    case ELifePhase::Roam:
        Agent.ActiveGoal = (Agent.AgentID % 2 == 0) ? FSimTags::Type_Park : FSimTags::Type_Shop;
        break;
    default:
        Agent.ActiveGoal = FSimTags::Type_Home;
        break;
    }
}

float ULifeSchedulerSubsystem::GetTimeDifference(float StartTime, float EndTime) const
{
    float Diff = EndTime - StartTime;
    if (Diff < 0.0f)
    {
        Diff += 24.0f;
    }
    return Diff;
}