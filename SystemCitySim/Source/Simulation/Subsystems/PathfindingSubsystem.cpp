#include "Simulation/Subsystems/PathfindingSubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Subsystems/RoadPathfindingSubsystem.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "NavigationPath.h"

void UPathfindingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}
// Async graph traversal wrapper. Offloads A* execution from the main thread to prevent frame stalls during mass agent phase transitions (e.g., Rush Hour).
void UPathfindingSubsystem::RequestPath(int32 AgentID, int32 StartNode, int32 EndNode)
{

    UE_LOG(LogSimulation, Warning, TEXT("[PATH REQUEST] Agent %d StartNode=%d EndNode=%d"), AgentID, StartNode, EndNode);
    URoadPathfindingSubsystem* RoadPath = GetWorld()->GetSubsystem<URoadPathfindingSubsystem>();

    FPathResult ResultData;
    ResultData.AgentID = AgentID;

    if (RoadPath)
    {
        TArray<FVector> Nodes = RoadPath->ComputePath(StartNode, EndNode);

        if (Nodes.Num() > 0)
        {
            ResultData.bSuccess = true;
            float SideMultiplier = (AgentID % 2 == 0) ? 1.f : -1.f;

            for (int32 i = 0; i < Nodes.Num(); ++i)
            {
                FVector ForwardDir = FVector::ForwardVector;
                if (i < Nodes.Num() - 1)
                {
                    ForwardDir = (Nodes[i + 1] - Nodes[i]).GetSafeNormal();
                }
                else if (i > 0)
                {
                    ForwardDir = (Nodes[i] - Nodes[i - 1]).GetSafeNormal();
                }

                FVector RightDir = FVector::CrossProduct(ForwardDir, FVector::UpVector).GetSafeNormal();
                Nodes[i] += (RightDir * 450.f * SideMultiplier);
              
            }

            ResultData.PathPoints = Nodes;
        }
        else
        {
            ResultData.bSuccess = false;
        }
    }
    else
    {
        ResultData.bSuccess = false;
    }

    FScopeLock Lock(&PathMutex);
    ResolvedPaths.Add(ResultData);
}

void UPathfindingSubsystem::OnPathResolved(
    uint32 PathID,
    ENavigationQueryResult::Type Result,
    FNavPathSharedPtr NavPointer,
    int32 AgentID)
{
    FPathResult ResultData;

    ResultData.AgentID = AgentID;
    ResultData.bSuccess =
        (Result == ENavigationQueryResult::Success) &&
        NavPointer.IsValid();

    if (ResultData.bSuccess)
    {
        for (const FNavPathPoint& Pt : NavPointer->GetPathPoints())
        {
            ResultData.PathPoints.Add(Pt.Location);
        }
    }

    FScopeLock Lock(&PathMutex);
    ResolvedPaths.Add(ResultData);
}

void UPathfindingSubsystem::ProcessResolvedPaths()
{
    UNeedsSubsystem* NeedsSub = GetWorld()->GetSubsystem<UNeedsSubsystem>();  
        UInteractionSubsystem* IntSub = GetWorld()->GetSubsystem<UInteractionSubsystem>(); 
        if (!NeedsSub || !IntSub) return;  
            FScopeLock Lock(&PathMutex);  
            for (const FPathResult& Res : ResolvedPaths) 
            {
                FAgentData* Agent = NeedsSub->GetAgentData(Res.AgentID);  
                    if (!Agent || Agent->State != EAgentState::WaitingForPath) continue;  

                        if (Res.bSuccess && Res.PathPoints.Num() > 0) 
                        {
                            Agent->CurrentPath = Res.PathPoints; 
                                Agent->State = EAgentState::Traveling;  
                                UE_LOG(LogSimulation, Log, TEXT("[Agent %d] PATH SUCCESS -> %d nodes. STATE -> Traveling"), Agent->AgentID, Res.PathPoints.Num());  
                        }
                        else  
                            {
                                
                                UE_LOG(LogAgentDebug, Warning, TEXT("[Agent %d] Path failed -> Reaction"), Agent->AgentID); 
                                    IntSub->ReleaseSlot(Agent->ActiveInteractableID, Agent->AgentID);  

                                    
                                    
                                    
                                    if (Agent->ActiveInteractableID != -1)
                                    {
                                        Agent->FailedTargets.Add(Agent->ActiveInteractableID);
                                    }

                                Agent->ActiveInteractableID = -1;  

                                    Agent->State = EAgentState::Reaction;  
                                    Agent->StateTimer = 1.5f;  
                            }
            }
    ResolvedPaths.Empty();  
}