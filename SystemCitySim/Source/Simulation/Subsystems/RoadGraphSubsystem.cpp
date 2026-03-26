#include "Simulation/Subsystems/RoadGraphSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"

void URoadGraphSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Nodes.Empty();
    Edges.Empty();
    NodeLocationHash.Empty();
}

int32 URoadGraphSubsystem::AddNode(const FVector& Location)
{
    FIntVector GridLoc(
        FMath::RoundToInt(Location.X),
        FMath::RoundToInt(Location.Y),
        0);

    uint64 Hash = GetTypeHash(GridLoc);

    if (int32* Found = NodeLocationHash.Find(Hash))
    {
        return *Found;
    }

    FRoadNode Node;

    Node.NodeID = Nodes.Num();
    Node.Location = Location;

    Nodes.Add(Node);

    NodeLocationHash.Add(Hash, Node.NodeID);

    return Node.NodeID;
}

int32 URoadGraphSubsystem::AddEdge(
    int32 StartNode,
    int32 EndNode,
    FVector StartLoc,
    FVector EndLoc,
    float InLength) 
{
    FRoadEdge Edge;
    Edge.EdgeID = Edges.Num(); 
    Edge.StartNodeID = StartNode; 
    Edge.EndNodeID = EndNode; 
    Edge.Length = InLength; 
    Edge.StartLocation = StartLoc;
    Edge.EndLocation = EndLoc;

    Edges.Add(Edge); 

    Nodes[StartNode].ConnectedEdges.Add(Edge.EdgeID); 
    Nodes[EndNode].ConnectedEdges.Add(Edge.EdgeID); 
    UE_LOG(LogSimulation, Warning, TEXT("[GRAPH] Edge Added: %d <-> %d (Length=%.2f)"), StartNode, EndNode, InLength);
    return Edge.EdgeID; 
}

const FRoadNode* URoadGraphSubsystem::GetNode(int32 NodeID) const
{
    return Nodes.IsValidIndex(NodeID) ? &Nodes[NodeID] : nullptr;
}

const FRoadEdge* URoadGraphSubsystem::GetEdge(int32 EdgeID) const
{
    return Edges.IsValidIndex(EdgeID) ? &Edges[EdgeID] : nullptr;
}

 

bool URoadGraphSubsystem::ValidateGraphConnectivity()
{
    if (Nodes.Num() == 0)
    {
        UE_LOG(LogSimulation, Warning, TEXT("[GRAPH] No nodes"));
        return true;
    }

    TSet<int32> Visited;
    TArray<int32> Queue;

    Queue.Add(0);
    Visited.Add(0);

    while (Queue.Num() > 0)
    {
        int32 Current = Queue[0];
        Queue.RemoveAt(0);

        const FRoadNode& Node = Nodes[Current];

        for (int32 EdgeID : Node.ConnectedEdges)
        {
            const FRoadEdge& Edge = Edges[EdgeID];

            int32 Neighbor =
                (Edge.StartNodeID == Current)
                ? Edge.EndNodeID
                : Edge.StartNodeID;

            if (!Visited.Contains(Neighbor))
            {
                Visited.Add(Neighbor);
                Queue.Add(Neighbor);
            }
        }
    }

    UE_LOG(LogSimulation, Warning,
        TEXT("[GRAPH] Connected: %d / Total: %d"),
        Visited.Num(),
        Nodes.Num());

    if (Nodes.IsValidIndex(13))
    {
        UE_LOG(LogSimulation, Warning,
            TEXT("[GRAPH DEBUG] Node 13 connections: %d"),
            Nodes[13].ConnectedEdges.Num());
    }

    if (Visited.Num() != Nodes.Num())
    {
        UE_LOG(LogSimulation, Error,
            TEXT("[GRAPH ERROR] DISCONNECTED (%d isolated nodes)"),
            Nodes.Num() - Visited.Num());

        return false;
    }

    return true;
}