#include "Simulation/Subsystems/SimAISubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Subsystems/PathfindingSubsystem.h"
#include "Simulation/Subsystems/RoadPathfindingSubsystem.h"
#include "Simulation/Subsystems/VehicleSimulationSubsystem.h"
#include "Simulation/Subsystems/TransitSubsystem.h"
#include "Simulation/SimulationTags.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"

void USimAISubsystem::ProcessTick(ESimPhase Phase, float WorldTime)
{
    UNeedsSubsystem* NeedsSub = GetWorld()->GetSubsystem<UNeedsSubsystem>();
    UInteractionSubsystem* IntSub = GetWorld()->GetSubsystem<UInteractionSubsystem>();
    UPathfindingSubsystem* PathSub = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
    URoadPathfindingSubsystem* RoadPath = GetWorld()->GetSubsystem<URoadPathfindingSubsystem>();

    if (!NeedsSub || !IntSub || !PathSub || !RoadPath) return;

    static int32 StartupTicks = 0;
    if (StartupTicks < 30)
    {
        StartupTicks++;
        return;
    }

    const FGameplayTag TagHome = FSimTags::Type_Home;
    const FGameplayTag TagOffice = FSimTags::Type_Office;
    const FGameplayTag TagCafe = FSimTags::Type_Cafe;
    const FGameplayTag TagShop = FSimTags::Type_Shop;
    const FGameplayTag TagPark = FSimTags::Type_Park;

    for (FAgentData& Agent : NeedsSub->GetActiveAgents())
    {
        if (Agent.State != EAgentState::Decision)
            continue;

        if (Agent.CurrentPhase == ELifePhase::Sleep)
        {
            Agent.DesiredGoal = TagHome;
            Agent.DecisionReason = TEXT("Mandatory Sleep Phase");

            if (Agent.bInsideBuilding && Agent.ActiveInteractableID == Agent.HomeObjectID)
            {
                continue;
            }
        }
        else
        {
            switch (Agent.AgentType)
            {
            case EAgentType::Worker:
            {
                Agent.DesiredGoal = Agent.ActiveGoal;
                break;
            }
            case EAgentType::Civilian:
            {
                
                int32 SimTimeBucket = FMath::FloorToInt(WorldTime);
                int32 Hash = (Agent.AgentID * 37 + SimTimeBucket) % 100;
                Agent.DesiredGoal = (Hash < 40) ? TagCafe : ((Hash < 70) ? TagPark : TagShop);
                break;
            }
            case EAgentType::Wanderer:
            {
                float Angle = Agent.AgentID * 137.5f;
                FVector Offset = FVector(
                    FMath::Cos(Angle) * 2000.f,
                    FMath::Sin(Angle) * 2000.f,
                    0.f
                );
                Agent.TargetLocation = Agent.LogicalLocation + Offset;
                Agent.TargetLocation.Z = Agent.LogicalLocation.Z;

                Agent.bInsideBuilding = false;
                int32 StartNode = RoadPath->GetNearestNode(Agent.LogicalLocation);
                int32 TargetNode = RoadPath->GetNearestNode(Agent.TargetLocation);
                Agent.DecisionReason = TEXT("Wandering");
                Agent.State = EAgentState::WaitingForPath;
                PathSub->RequestPath(Agent.AgentID, StartNode, TargetNode);
                continue;
            }
            }
        }

        UWorldSimulationSubsystem* SimSub = GetWorld()->GetSubsystem<UWorldSimulationSubsystem>();
        if (SimSub && SimSub->bIsRaining && !Agent.bInsideBuilding)
        {
            if (Agent.DesiredGoal == FSimTags::Type_Park)
            {
                if (Agent.AgentID % 10 < 8)
                {
                    Agent.DesiredGoal = (Agent.AgentID % 2 == 0) ? FSimTags::Type_Cafe : FSimTags::Type_Home;
                    Agent.DecisionReason = TEXT("Rain Shelter");
                }
            }
        }

        
        if (Agent.PreviousGoal != Agent.DesiredGoal)
        {
            Agent.CurrentPath.Empty();
            Agent.TargetLocation = FVector::ZeroVector;
            Agent.bInsideBuilding = false;
        }
        Agent.PreviousGoal = Agent.DesiredGoal;

        int32 ResolvedTarget = -1;
        if (Agent.DesiredGoal == TagHome)
        {
            ResolvedTarget = Agent.HomeObjectID;
        }
        else if (Agent.DesiredGoal == TagOffice)
        {
            ResolvedTarget = Agent.WorkObjectID;
            if (Agent.CurrentPhase == ELifePhase::Work) Agent.DecisionReason = TEXT("Going to Work");
        }
        else
        {
            ResolvedTarget = IntSub->FindBestObjectForGoal(
                Agent.DesiredGoal,
                Agent.LogicalLocation,
                Agent.FailedTargets, Agent.ActiveInteractableID
            );

            if (Agent.DecisionReason.IsEmpty())
            {
                if (Agent.DesiredGoal == TagCafe) Agent.DecisionReason = TEXT("Need Coffee");
                else if (Agent.DesiredGoal == TagShop) Agent.DecisionReason = TEXT("Shopping");
                else if (Agent.DesiredGoal == TagPark) Agent.DecisionReason = TEXT("Going to Park");
            }
        }

        if (ResolvedTarget == -1)
        {
            if (Agent.DesiredGoal != TagHome && Agent.DesiredGoal != TagOffice)
            {
                
                Agent.DesiredGoal = (Agent.AgentID % 2 == 0) ? TagPark : TagHome;
                Agent.ActiveGoal = Agent.DesiredGoal;
                Agent.State = EAgentState::Decision;
            }
            else
            {
                Agent.State = EAgentState::Idle;
                Agent.StateTimer = 4.0f;
            }
            continue;
        }

        Agent.DesiredTargetID = ResolvedTarget;
        Agent.ActiveInteractableID = ResolvedTarget;

        Agent.TargetLocation = IntSub->GetInsideLocation(ResolvedTarget);
        Agent.TargetLocation.Z = Agent.LogicalLocation.Z;
        Agent.FinalInsideLocation = Agent.TargetLocation;
        Agent.bInsideBuilding = false;

        
        if (Agent.CurrentPath.IsEmpty())
        {
            if (FVector::DistSquared(Agent.LogicalLocation, Agent.TargetLocation) < 40000.f)
            {
                Agent.State = EAgentState::FinalApproach;
                continue;
            }

            int32 StartNode = RoadPath->GetNearestNode(Agent.LogicalLocation);
            int32 TargetNode = RoadPath->GetNearestNode(Agent.TargetLocation);

            Agent.State = EAgentState::WaitingForPath;
            PathSub->RequestPath(Agent.AgentID, StartNode, TargetNode);
        }
    }
}
 

 

TArray<FSmartObjectEntry*> USimAISubsystem::FindBestSmartObjects(const FAgentData& Agent, FGameplayTag ObjectType) {
    UInteractionSubsystem* IntSub = GetWorld()->GetSubsystem<UInteractionSubsystem>();
    TArray<FSmartObjectEntry*> Valid;
    if (!IntSub) return Valid;

    for (const FSmartObjectEntry& Entry : IntSub->GetAvailableObjects()) {
        if (Entry.Component && Entry.Component->ObjectType == ObjectType) {
            if (Entry.Component->CurrentUsers < Entry.Component->MaxUsers && !Agent.FailedTargets.Contains(Entry.ObjectID)) {
                Valid.Add(IntSub->GetSmartObjectByID(Entry.ObjectID));
            }
        }
    }
    return Valid;
}

void USimAISubsystem::EvaluateCommitment(FAgentData& Agent, float DeltaTime)
{
    if (Agent.DecisionCooldown > 0.f) 
    {
        Agent.DecisionCooldown -= DeltaTime; 
    }

    if (Agent.bIsCommittedToGoal) 
    {
        if (Agent.CommitmentLockTime > 0.f) 
        {
            Agent.CommitmentLockTime -= DeltaTime; 
        }

        
        bool bPhaseRequiresHold = false;
        if (Agent.bInsideBuilding && Agent.State == EAgentState::Interacting)
        {
            if (Agent.CurrentPhase == ELifePhase::Work && Agent.ActiveGoal == FSimTags::Type_Office) bPhaseRequiresHold = true;
            if (Agent.CurrentPhase == ELifePhase::Sleep && Agent.ActiveGoal == FSimTags::Type_Home) bPhaseRequiresHold = true;
            if (Agent.CurrentPhase == ELifePhase::CoffeeBreak && Agent.ActiveGoal == FSimTags::Type_Cafe) bPhaseRequiresHold = true;
        }

        if (bPhaseRequiresHold)
        {
            Agent.CommitmentLockTime = FMath::Max(Agent.CommitmentLockTime, 1.0f);
            if (Agent.CommitmentLockTime <= 1.0f)
            {
                UE_LOG(LogSimulation, Verbose, TEXT("[Agent %d] PhaseHold: Staying in %s"), Agent.AgentID, *Agent.ActiveGoal.ToString());
            }
            return;
        }

        
        if (Agent.State == EAgentState::Reaction || Agent.CommitmentLockTime <= 0.f) 
        {
            Agent.bIsCommittedToGoal = false; 
            UE_LOG(LogSimulation, Log, TEXT("[Agent %d] Commitment Released"), Agent.AgentID);
        }
    }
}

void USimAISubsystem::MakeDecision(FAgentData& Agent, float WorldTime)
{
    if (Agent.bIsCommittedToGoal || Agent.DecisionCooldown > 0.f) return;

    
    if (Agent.PreviousGoal != Agent.DesiredGoal)
    {
        Agent.CurrentPath.Empty();
        Agent.TargetLocation = FVector::ZeroVector;
        Agent.bInsideBuilding = false; 
    }
    Agent.PreviousGoal = Agent.DesiredGoal;

    UInteractionSubsystem* IntSub = GetWorld()->GetSubsystem<UInteractionSubsystem>();
    URoadPathfindingSubsystem* RoadPath = GetWorld()->GetSubsystem<URoadPathfindingSubsystem>();
    UPathfindingSubsystem* PathSub = GetWorld()->GetSubsystem<UPathfindingSubsystem>();

    int32 ResolvedTarget = -1;

    
    if (Agent.DesiredGoal == FSimTags::Type_Home)
    {
        ResolvedTarget = Agent.HomeObjectID;
    }
    else if (Agent.DesiredGoal == FSimTags::Type_Office)
    {
        ResolvedTarget = Agent.WorkObjectID;
    }
    else
    {
        ResolvedTarget = IntSub->FindBestObjectForGoal(Agent.DesiredGoal, Agent.LogicalLocation, Agent.FailedTargets, Agent.ActiveInteractableID);
    }

    FVector EntryLoc = IntSub->GetObjectEntryLocation(ResolvedTarget);

    if (ResolvedTarget == -1 || Agent.FailedTargets.Contains(ResolvedTarget) || EntryLoc.IsNearlyZero())
    {
        ResolvedTarget = IntSub->FindBestObjectForGoal(Agent.DesiredGoal, Agent.LogicalLocation, Agent.FailedTargets, -1);
        if (ResolvedTarget == -1)
        {
            
            ResolvedTarget = IntSub->FindBestObjectForGoal(FSimTags::Type_Cafe, Agent.LogicalLocation, Agent.FailedTargets, -1);
            if (ResolvedTarget != -1)
            {
                Agent.DesiredGoal = FSimTags::Type_Cafe;
                Agent.DecisionReason = TEXT("Emergency Shelter (Goal Invalid)");
            }
            else
            {
                
                Agent.DecisionReason = TEXT("Wandering Failsafe");

                
                float Angle = Agent.AgentID * 137.5f;
                Agent.TargetLocation = Agent.LogicalLocation + FVector(FMath::Cos(Angle) * 2000.f, FMath::Sin(Angle) * 2000.f, 0.f);
                Agent.TargetLocation.Z = Agent.LogicalLocation.Z;
                Agent.State = EAgentState::WaitingForPath;
                Agent.bIsCommittedToGoal = true;
                Agent.CommitmentLockTime = 15.0f;

                int32 StartNode = RoadPath->GetNearestNode(Agent.LogicalLocation);
                int32 TargetNode = RoadPath->GetNearestNode(Agent.TargetLocation);
                PathSub->RequestPath(Agent.AgentID, StartNode, TargetNode);
                return;
            }
        }
        else if (Agent.DesiredGoal == FSimTags::Type_Office)
        {
            Agent.WorkObjectID = ResolvedTarget;
        }

        EntryLoc = IntSub->GetObjectEntryLocation(ResolvedTarget);
        if (EntryLoc.IsNearlyZero())
        {
            EntryLoc = IntSub->GetObjectLocation(ResolvedTarget);
        }
    }

    Agent.DesiredTargetID = ResolvedTarget;
    Agent.ActiveInteractableID = ResolvedTarget;

    Agent.TargetLocation = EntryLoc;
    Agent.TargetLocation.Z = 0.0f;
    Agent.FinalInsideLocation = IntSub->GetInsideLocation(ResolvedTarget);
    Agent.bInsideBuilding = false;

    
    if (Agent.CurrentPath.IsEmpty())
    {
        int32 StartNode = RoadPath->GetNearestNode(Agent.LogicalLocation);
        int32 TargetNode = RoadPath->GetNearestNode(Agent.TargetLocation);

        Agent.State = EAgentState::WaitingForPath;
        Agent.StateTimer = 0.0f;
        Agent.bIsCommittedToGoal = true;
        Agent.CommitmentLockTime = 60.0f;

        PathSub->RequestPath(Agent.AgentID, StartNode, TargetNode);
    }
}


void USimAISubsystem::ExecuteMovement(FAgentData& Agent, float DeltaTime, const TArray<FAgentData>& AllAgents)
{
    if (Agent.bInsideBuilding)
    {
        Agent.SimVelocity = FVector::ZeroVector;
        Agent.CurrentAnimationState = TEXT("Idle");
        return;
    }

    if (Agent.CurrentPath.Num() > 0 && Agent.State == EAgentState::Idle)
    {
        Agent.State = EAgentState::Traveling;
    }

    if (Agent.State == EAgentState::Decision || Agent.State == EAgentState::WaitingForPath ||
        Agent.State == EAgentState::Idle || Agent.State == EAgentState::WaitingForBus)
    {
        Agent.SimVelocity = FVector::ZeroVector;
        Agent.CurrentAnimationState = TEXT("Idle");
        return;
    }

    UWorldSimulationSubsystem* SimSub = GetWorld()->GetSubsystem<UWorldSimulationSubsystem>();
    float MaxSpeed = 250.f + (Agent.AgentID % 20);

    if (SimSub && SimSub->bIsRaining && !Agent.bInsideBuilding)
    {
        MaxSpeed *= 1.8f;
    }

    const float TickStepDistance = MaxSpeed * DeltaTime;
    FVector DesiredVelocity = FVector::ZeroVector;

    if (Agent.State == EAgentState::Traveling)
    {
        if (Agent.CurrentPath.Num() == 0)
        {
            
            
            if (FVector::DistSquared(Agent.LogicalLocation, Agent.TargetLocation) > FMath::Square(5000.f))
            {
                int32 StartNode = GetWorld()->GetSubsystem<URoadPathfindingSubsystem>()->GetNearestNode(Agent.LogicalLocation);
                int32 TargetNode = GetWorld()->GetSubsystem<URoadPathfindingSubsystem>()->GetNearestNode(Agent.TargetLocation);
                Agent.State = EAgentState::WaitingForPath;
                GetWorld()->GetSubsystem<UPathfindingSubsystem>()->RequestPath(Agent.AgentID, StartNode, TargetNode);
                return;
            }

            
            UInteractionSubsystem* IntSub = GetWorld()->GetSubsystem<UInteractionSubsystem>();
            if (IntSub)
            {
                FInteractionSlot* Slot = IntSub->ReserveSlot(Agent.ActiveInteractableID, Agent.AgentID);
                if (!Slot)
                {
                    Agent.FailedTargets.Add(Agent.ActiveInteractableID);
                    Agent.State = EAgentState::Reaction;
                    Agent.StateTimer = 1.5f;
                    Agent.bIsCommittedToGoal = false;
                    Agent.SimVelocity = FVector::ZeroVector;
                    Agent.CurrentPath.Empty();
                    return;
                }

                Agent.State = EAgentState::FinalApproach;
                Agent.TargetLocation = Slot->Location;
                Agent.TargetLocation.Z = Agent.LogicalLocation.Z;
            }
            else
            {
                Agent.State = EAgentState::FinalApproach;
            }
        }
        else
        {
            const FVector NextWaypoint = Agent.CurrentPath[0];
            FVector Dir = NextWaypoint - Agent.LogicalLocation;
            Dir.Z = 0.0f;
            float Dist = Dir.Size();

            if (Dist <= FMath::Max(100.f, TickStepDistance * 1.2f))
            {
                Agent.CurrentPath.RemoveAt(0);

                if (Agent.CurrentPath.Num() == 0)
                {
                    UInteractionSubsystem* IntSub = GetWorld()->GetSubsystem<UInteractionSubsystem>();
                    if (IntSub)
                    {
                        FInteractionSlot* Slot = IntSub->ReserveSlot(Agent.ActiveInteractableID, Agent.AgentID);
                        if (!Slot)
                        {
                            Agent.FailedTargets.Add(Agent.ActiveInteractableID);
                            Agent.State = EAgentState::Reaction;
                            Agent.StateTimer = 1.5f;
                            Agent.bIsCommittedToGoal = false;
                            Agent.SimVelocity = FVector::ZeroVector;
                            Agent.CurrentPath.Empty();
                            return;
                        }

                        Agent.State = EAgentState::FinalApproach;
                        Agent.TargetLocation = Slot->Location;
                        Agent.TargetLocation.Z = Agent.LogicalLocation.Z;
                    }
                    else
                    {
                        Agent.State = EAgentState::FinalApproach;
                    }
                }
            }
            else
            {
                DesiredVelocity = Dir.GetSafeNormal() * MaxSpeed;
            }
        }
    }
    else if (Agent.State == EAgentState::FinalApproach)
    {
        FVector Dir = Agent.TargetLocation - Agent.LogicalLocation;
        Dir.Z = 0.0f;
        float Dist = Dir.Size();

        if (Dist <= FMath::Max(50.f, TickStepDistance * 1.5f))
        {
            
            Agent.LogicalLocation = Agent.TargetLocation;
            Agent.SimVelocity = FVector::ZeroVector;
            Agent.State = EAgentState::Interacting;
            Agent.bInsideBuilding = true;
            Agent.bIsCommittedToGoal = false;

            Agent.InteractionRemainingTime = FMath::RandRange(15.0f, 35.0f);
            return;
        }
        else
        {
            Agent.LogicalLocation += Dir.GetSafeNormal() * MaxSpeed * DeltaTime;
            Agent.SimVelocity = Dir.GetSafeNormal() * MaxSpeed;
            return;
        }
    }

    
    FVector Steering = DesiredVelocity - Agent.SimVelocity;
    Steering = Steering.GetClampedToMaxSize(150.f);
    Agent.SimVelocity += Steering * DeltaTime;
    Agent.LogicalLocation += Agent.SimVelocity * DeltaTime;
    Agent.LogicalLocation.Z = 0.0f;

#if !(UE_BUILD_SHIPPING)
    const bool bDrawDebug = true;
    if (bDrawDebug)
    {
        const FColor AgentColor((Agent.AgentID * 83) % 255, (Agent.AgentID * 197) % 255, (Agent.AgentID * 113) % 255);
        FVector DrawTarget = FVector::ZeroVector;
        bool bShouldDraw = false;

        if (Agent.State == EAgentState::Traveling && Agent.CurrentPath.Num() > 0)
        {
            DrawTarget = Agent.CurrentPath[0];
            bShouldDraw = true;
        }
        else if (Agent.State == EAgentState::FinalApproach)
        {
            DrawTarget = Agent.TargetLocation;
            bShouldDraw = true;
        }

        if (bShouldDraw)
        {
            DrawTarget.Z = Agent.LogicalLocation.Z;
            DrawDebugLine(GetWorld(), Agent.LogicalLocation, DrawTarget, AgentColor, false, 0.0f, 0, 4.0f);
            DrawDebugSphere(GetWorld(), DrawTarget, 30.0f, 8, AgentColor, false, 0.0f, 0, 1.5f);
        }
    }
#endif
}