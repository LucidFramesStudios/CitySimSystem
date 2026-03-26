#include "Simulation/Subsystems/LifeAssignmentSubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Subsystems/SimulationRandomSubsystem.h"

void ULifeAssignmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}
// Two-pass deterministic assignment. The fallback loop enforces a critical invariant: no agent exists without a valid HomeObjectID, preventing undefined behavior in the scheduling phase.
void ULifeAssignmentSubsystem::ExecuteLifeAssignments()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UNeedsSubsystem* Needs = World->GetSubsystem<UNeedsSubsystem>();
    UInteractionSubsystem* IntSub = World->GetSubsystem<UInteractionSubsystem>();
    USimulationRandomSubsystem* Rand = World->GetSubsystem<USimulationRandomSubsystem>();

    if (!Needs || !IntSub || !Rand)
    {
        UE_LOG(LogSimulation, Error, TEXT("LifeAssignment failed: missing subsystems"));
        return;
    }

    
    
    
    AssignFacilities(IntSub, Needs, Rand);

    
    
    
    GenerateFriendNetworks(Needs, Rand);

    
    
    
    TSet<int32> Empty;

    for (FAgentData& Agent : Needs->GetActiveAgents())
    {
        if (Agent.HomeObjectID != -1)
            continue;

        UE_LOG(LogSimulation, Error,
            TEXT("Agent %d had NO HOME → assigning fallback"), Agent.AgentID);

        int32 HomeID = IntSub->FindBestObjectForGoal(
            FSimTags::Type_Home,
            FVector::ZeroVector,
            Empty, Agent.ActiveInteractableID
        );

        if (HomeID == -1)
        {
            UE_LOG(LogSimulation, Fatal, TEXT("NO HOME OBJECTS IN WORLD"));
        }

        Agent.HomeObjectID = HomeID;
    }

    
    
    
    for (const FAgentData& Agent : Needs->GetActiveAgents())
    {
        if (Agent.HomeObjectID == -1)
        {
            UE_LOG(LogSimulation, Fatal,
                TEXT("Agent %d STILL has no home"), Agent.AgentID);
        }
    }

    UE_LOG(LogSimulation, Log, TEXT("LifeAssignment complete (homes guaranteed)"));
}


void ULifeAssignmentSubsystem::AssignFacilities(
    UInteractionSubsystem* IntSub,
    UNeedsSubsystem* Needs,
    USimulationRandomSubsystem* Rand)
{
    TArray<int32> Homes;
    TArray<int32> Workplaces;
    TArray<int32> FallbackObjects;

    for (const FSmartObjectEntry& Entry : IntSub->GetAvailableObjects())
    {
        if (!Entry.Component) continue;

        FallbackObjects.Add(Entry.ObjectID);

        if (Entry.Component->ObjectType.MatchesTag(FGameplayTag::RequestGameplayTag("Type.Home")))
        {
            Homes.Add(Entry.ObjectID);
        }
        else if (Entry.Component->ObjectType.MatchesTag(FGameplayTag::RequestGameplayTag("Type.Office")))
        {
            Workplaces.Add(Entry.ObjectID);
        }
    }

    if (FallbackObjects.IsEmpty())
    {
        UE_LOG(LogSimulation, Fatal, TEXT("No SmartObjects found."));
        return;
    }

    TArray<FAgentData>& Agents = Needs->GetActiveAgents();

    for (FAgentData& Agent : Agents)
    {
        
        
        
        if (Agent.HomeObjectID == -1)
        {
            if (Homes.Num() > 0)
            {
                int32 ClusterOffset = (Agent.AgentID / 20) % Homes.Num();
                Agent.HomeObjectID = Homes[(Agent.AgentID + ClusterOffset) % Homes.Num()];
            }
            else
            {
                Agent.HomeObjectID = FallbackObjects[Agent.AgentID % FallbackObjects.Num()];
            }
        }

        
        
        
        if (Agent.AgentType == EAgentType::Worker && Agent.WorkObjectID == -1)
        {
            if (Workplaces.Num() > 0)
            {
                Agent.WorkObjectID = Workplaces[(Agent.AgentID + 7) % Workplaces.Num()];
            }
            else
            {
                Agent.WorkObjectID = FallbackObjects[(Agent.AgentID + 7) % FallbackObjects.Num()];
            }
        }

        
        
        
        
        
        
    }
}

void ULifeAssignmentSubsystem::GenerateFriendNetworks(UNeedsSubsystem* Needs, USimulationRandomSubsystem* Rand)
{
    TArray<FAgentData>& Agents = Needs->GetActiveAgents();
    int32 NumAgents = Agents.Num();

    for (FAgentData& Agent : Agents)
    {
        
        int32 TargetFriends = FMath::RoundToInt(Agent.Personality.Extroversion * 5.f);
        for (int32 i = 0; i < TargetFriends; ++i)
        {
            int32 FriendID = Rand->RandInt(0, NumAgents - 1);
            if (FriendID != Agent.AgentID && !Agent.Friends.Contains(FriendID))
            {
                Agent.Friends.Add(FriendID);
            }
        }
    }
}