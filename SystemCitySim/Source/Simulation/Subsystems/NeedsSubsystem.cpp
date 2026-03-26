#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "Simulation/Actors/CityGeneratorActor.h"
#include "Simulation/Subsystems/CityEventSubsystem.h"
#include "EngineUtils.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Engine/World.h"

void UNeedsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ActiveAgents.Empty();
    SimRandomStream.Initialize(12345);

    const int32 NumAgents = InitialAgentCount;

    for (int32 i = 0; i < NumAgents; i++)
    {
        FAgentData Agent;
        Agent.AgentID = i;

        float Rand = SimRandomStream.FRand();

        if (Rand < 0.7f)
            Agent.AgentType = EAgentType::Worker;
        else
            Agent.AgentType = EAgentType::Civilian;

        
        Agent.LogicalLocation = FVector::ZeroVector;
        Agent.TargetLocation = FVector::ZeroVector;

        Agent.State = EAgentState::Decision;
        Agent.StateTimer = 0.f;

        Agent.bInsideBuilding = false;
        Agent.HomeObjectID = -1;
        Agent.WorkObjectID = -1;
        Agent.ActiveInteractableID = -1;

        ActiveAgents.Add(Agent);
    }

    UE_LOG(LogSimulation, Warning, TEXT("NeedsSubsystem initialized with %d agents"), NumAgents);
}





void UNeedsSubsystem::ProcessTick(float DeltaTime, ESimPhase Phase)
{
    UWorld* World = GetWorld();
    if (!World) return;

    
    for (FAgentData& Agent : ActiveAgents)
    {
        
        if (Agent.State == EAgentState::Traveling ||
            Agent.State == EAgentState::FinalApproach ||
            Agent.State == EAgentState::WaitingForPath)
        {
            Agent.StateTimer += DeltaTime; 

            if (Agent.StateTimer > 40.0f) 
            {
                Agent.State = EAgentState::Reaction;
                Agent.StateTimer = 1.0f;
                Agent.bIsCommittedToGoal = false;
                Agent.FailedTargets.Add(Agent.ActiveInteractableID);
                Agent.CurrentPath.Empty();

                if (UInteractionSubsystem* IntSub = World->GetSubsystem<UInteractionSubsystem>())
                {
                    IntSub->ReleaseSlot(Agent.ActiveInteractableID, Agent.AgentID);
                }
            }
        }
        
        else if (Agent.State == EAgentState::Idle || Agent.State == EAgentState::Reaction || Agent.State == EAgentState::Cooldown || Agent.State == EAgentState::Decision) 
        {
            Agent.StateTimer -= DeltaTime; 

            
            if (Agent.StateTimer <= -10.0f)
            {
                SIM_LOG_WARNING(Agent.AgentID, TEXT("Watchdog: Agent deadlocked in Idle/Decision. Forcing hard reset."));
                Agent.State = EAgentState::Decision; 
                Agent.StateTimer = 1.0f; 
                Agent.bIsCommittedToGoal = false; 
                Agent.FailedTargets.Empty(); 
                Agent.DecisionCooldown = 0.f; 
            }
            else if (Agent.StateTimer <= 0.f && Agent.State != EAgentState::Decision) 
            {
                Agent.State = EAgentState::Decision; 
                Agent.DesiredTargetID = -1; 
            }
        }

        if (const float* EnergyNeed = Agent.Needs.Find(FSimTags::Need_Energy))
        {
            if (*EnergyNeed < 0.2f) Agent.ExpressionState.CurrentExpression = EAgentExpression::Tired;
            else Agent.ExpressionState.CurrentExpression = EAgentExpression::Neutral;
        }
        else if (Agent.State == EAgentState::Idle || Agent.State == EAgentState::Decision || Agent.State == EAgentState::Reaction)
        {
            Agent.ExpressionState.CurrentExpression = EAgentExpression::Confused;
        }
        else
        {
            Agent.ExpressionState.CurrentExpression = EAgentExpression::Neutral;
        }
    }
}

FAgentData* UNeedsSubsystem::GetAgentData(int32 AgentID)
{
    if (ActiveAgents.IsValidIndex(AgentID))
    {
        return &ActiveAgents[AgentID];
    }
    return nullptr;
}






int32 UNeedsSubsystem::FindClosestEdgeToLocation(const FVector& Location, const TArray<FRoadEdge>& Edges) const
{
    return -1;
}

int32 UNeedsSubsystem::FindClosestSidewalkSegment(const FVector& Location, const TArray<struct FSidewalkSegment>& Segments) const
{
    return -1;
}

void UNeedsSubsystem::ScoreAgentNeeds(FAgentData& Agent, float DeltaTime)
{
    for (auto& NeedPair : Agent.Needs)
    {
        NeedPair.Value = FMath::Clamp(NeedPair.Value - (0.01f * DeltaTime), 0.0f, 1.0f);
    }

    if (Agent.State != EAgentState::Decision)
    {
        return;
    }

    Agent.DesiredGoal = Agent.ActiveGoal;

    
    if (Agent.CurrentPhase == ELifePhase::Work ||
        Agent.CurrentPhase == ELifePhase::Sleep ||
        Agent.CurrentPhase == ELifePhase::WakeUp ||
        Agent.CurrentPhase == ELifePhase::CommuteToWork ||
        Agent.CurrentPhase == ELifePhase::CommuteHome)
    {
        if (Agent.CurrentPhase == ELifePhase::Work) Agent.DecisionReason = TEXT("On the Clock");
        else if (Agent.CurrentPhase == ELifePhase::Sleep) Agent.DecisionReason = TEXT("Mandatory Rest");
        else if (Agent.CurrentPhase == ELifePhase::WakeUp) Agent.DecisionReason = TEXT("Morning Routine");
        return;
    }

    float LowestScore = 1.0f;

    
    if (Agent.CurrentPhase == ELifePhase::Relax || Agent.CurrentPhase == ELifePhase::Roam)
    {
        UWorldSimulationSubsystem* SimSub = GetWorld()->GetSubsystem<UWorldSimulationSubsystem>();
        int32 SimTimeBucket = SimSub ? FMath::FloorToInt(SimSub->GetWorldTime()) : 0;

        
        int32 Hash = (Agent.AgentID * 37 + SimTimeBucket) % 100;

        if (Hash < 40)
        {
            Agent.DesiredGoal = FSimTags::Type_Cafe;
            Agent.DecisionReason = TEXT("Leisure: Cafe");
        }
        else if (Hash < 70)
        {
            Agent.DesiredGoal = FSimTags::Type_Park;
            Agent.DecisionReason = TEXT("Leisure: Park");
        }
        else
        {
            Agent.DesiredGoal = FSimTags::Type_Shop;
            Agent.DecisionReason = TEXT("Leisure: Shop");
        }
    }

    
    if (const float* Hunger = Agent.Needs.Find(FSimTags::Need_Hunger))
    {
        if (*Hunger < 0.2f && *Hunger < LowestScore)
        {
            LowestScore = *Hunger;
            Agent.DesiredGoal = FSimTags::Type_Cafe;
            Agent.DecisionReason = TEXT("Critical Hunger (Leisure)");
        }
    }

    if (const float* Energy = Agent.Needs.Find(FSimTags::Need_Energy))
    {
        if (*Energy < 0.15f && *Energy < LowestScore)
        {
            LowestScore = *Energy;
            Agent.DesiredGoal = FSimTags::Type_Home;
            Agent.DecisionReason = TEXT("Critical Exhaustion (Leisure)");
        }
    }

    if (const float* Social = Agent.Needs.Find(FSimTags::Need_Social))
    {
        float SocialThreshold = 0.3f + (Agent.Personality.Extroversion * 0.2f);
        if (Agent.CurrentPhase == ELifePhase::Relax) SocialThreshold += 0.2f;

        if (*Social < SocialThreshold && *Social < LowestScore)
        {
            Agent.DesiredGoal = FSimTags::Type_Park;
            Agent.DecisionReason = TEXT("Social Seeking (Leisure)");
        }
    }
}