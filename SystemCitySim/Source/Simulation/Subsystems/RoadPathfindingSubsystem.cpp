#include "Simulation/Subsystems/RoadPathfindingSubsystem.h"
#include "Simulation/Subsystems/RoadGraphSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "DrawDebugHelpers.h"

void URoadPathfindingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    OpenSet.Reserve(2000);
    CameFrom.Reserve(2000);
    GScore.Reserve(2000);
    FScore.Reserve(2000);
}

int32 URoadPathfindingSubsystem::GetNearestNode(
    const FVector& Location) const
{
    URoadGraphSubsystem* RoadGraph =
        GetWorld()->GetSubsystem<URoadGraphSubsystem>();

    if (!RoadGraph)
        return -1;

    int32 BestNode = -1;

    float BestDist = MAX_FLT;

    for (const FRoadEdge& Edge : RoadGraph->GetAllEdges())
    {
        float Dist =
            FVector::DistSquared(
                Location,
                Edge.StartLocation);

        if (Dist < BestDist)
        {
            BestDist = Dist;
            BestNode = Edge.StartNodeID;
        }
    }

    return BestNode;
}


TArray<FVector> URoadPathfindingSubsystem::ComputePath(int32 StartNode, int32 TargetNode)
{
    TArray<FVector> Path;
    URoadGraphSubsystem* Graph = GetWorld()->GetSubsystem<URoadGraphSubsystem>();

    if (!Graph || StartNode == -1 || TargetNode == -1)
    {
        UE_LOG(LogSimulation, Error, TEXT("Pathfinder ABORTED: Invalid Nodes. Start=%d, Target=%d"), StartNode, TargetNode);
        return Path;
    }

    if (StartNode == TargetNode)
    {
        Path.Add(Graph->GetNode(StartNode)->Location);
        UE_LOG(LogSimulation, Log, TEXT("Pathfinder SUCCESS [Immediate]: StartNode=%d, EndNode=%d, NodeCount=1"), StartNode, TargetNode);
        return Path;
    }

    OpenSet.Reset();
    CameFrom.Reset();
    GScore.Reset();
    FScore.Reset();

    OpenSet.Add(StartNode);
    GScore.Add(StartNode, 0.f);

    const FRoadNode* TargetNodeInfo = Graph->GetNode(TargetNode);
    if (!TargetNodeInfo) return Path;

    const FRoadNode* StartNodeInfo = Graph->GetNode(StartNode);
    FScore.Add(StartNode, FVector::Distance(StartNodeInfo->Location, TargetNodeInfo->Location));

    while (OpenSet.Num() > 0)
    {
        int32 BestIndex = 0;
        float BestScore = MAX_FLT;

        for (int32 i = 0; i < OpenSet.Num(); i++)
        {
            float Score = FScore.FindRef(OpenSet[i]);
            if (Score < BestScore)
            {
                BestScore = Score;
                BestIndex = i;
            }
        }

        int32 CurrentNode = OpenSet[BestIndex];

        if (CurrentNode == TargetNode)
        {
            int32 Trace = TargetNode;
            while (Trace != StartNode)
            {
                Path.Insert(Graph->GetNode(Trace)->Location, 0);
                Trace = CameFrom[Trace];
            }
            Path.Insert(Graph->GetNode(StartNode)->Location, 0);

            UE_LOG(LogSimulation, Log, TEXT("Pathfinder SUCCESS: StartNode=%d, EndNode=%d, NodeCount=%d"), StartNode, TargetNode, Path.Num());
            return Path;
        }

        OpenSet.RemoveAtSwap(BestIndex);
        const FRoadNode* Node = Graph->GetNode(CurrentNode);

        for (int32 EdgeID : Node->ConnectedEdges)
        {
            const FRoadEdge* Edge = Graph->GetEdge(EdgeID);
            if (!Edge) continue;

            int32 Neighbor = (Edge->StartNodeID == CurrentNode) ? Edge->EndNodeID : Edge->StartNodeID;
            const FRoadNode* NeighborNode = Graph->GetNode(Neighbor);

            float Tentative = GScore.FindRef(CurrentNode) + Edge->Length;
            if (!GScore.Contains(Neighbor) || Tentative < GScore[Neighbor])
            {
                CameFrom.Add(Neighbor, CurrentNode);
                GScore.Add(Neighbor, Tentative);
                FScore.Add(Neighbor, Tentative + FVector::Distance(NeighborNode->Location, TargetNodeInfo->Location));

                if (!OpenSet.Contains(Neighbor))
                {
                    OpenSet.Add(Neighbor);
                }
            }
        }
    }

    UE_LOG(LogSimulation, Error, TEXT("Pathfinder FAILED (No Route): StartNode=%d, EndNode=%d"), StartNode, TargetNode);
    return Path;
}