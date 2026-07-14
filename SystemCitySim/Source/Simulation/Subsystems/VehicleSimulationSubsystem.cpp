#include "Simulation/Subsystems/VehicleSimulationSubsystem.h"
#include "Simulation/Subsystems/RoadGraphSubsystem.h"
#include "Simulation/Subsystems/TransitSubsystem.h"
#include "Simulation/Actors/SimulationSpawner.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "Simulation/Subsystems/RoadPathfindingSubsystem.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

void UVehicleSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);  
    ActiveVehicles.Empty();  

    if (GetWorld())  
    {
        FTimerHandle Handle;   
        GetWorld()->GetTimerManager().SetTimer(Handle, this, &UVehicleSimulationSubsystem::InitializeRoads, 0.2f, false);  
    }
}

void UVehicleSimulationSubsystem::InitializeRoads()
{
    BuildRoadNetwork(); 
    /*VehicleIDToIndex.Empty();   */
    if (ActiveVehicles.Num() == 0) return;  

    for (TActorIterator<ASimulationSpawner> It(GetWorld()); It; ++It)  
    {
        It->SpawnVehicles(this);  
    }
}

void UVehicleSimulationSubsystem::BuildRoadNetwork()
{
    ActiveVehicles.Empty();  
    URoadGraphSubsystem* RoadGraph = GetWorld()->GetSubsystem<URoadGraphSubsystem>(); 
    if (!RoadGraph) return; 

    const int32 VehicleCount = 40;
    const TArray<FRoadEdge>& Edges = RoadGraph->GetAllEdges();  
    if (Edges.Num() == 0) return;  

    for (int32 i = 0; i < VehicleCount; i++)  
    {
        FVehicleData Vehicle;
        Vehicle.VehicleID = i;  
        Vehicle.Speed = FMath::RandRange(600.f, 900.f);  
        Vehicle.State = EVehicleState::Driving;   

        // Pick random start and end nodes
        const FRoadEdge& StartEdge = Edges[FMath::RandRange(0, Edges.Num() - 1)];  
        int32 StartNode = StartEdge.StartNodeID;
        int32 EndNode = Edges[FMath::RandRange(0, Edges.Num() - 1)].EndNodeID;

        if (GenerateRoute(Vehicle, StartNode, EndNode))
        {
            Vehicle.CurrentEdgeID = Vehicle.EdgeRoute[0];
            Vehicle.CurrentRouteIndex = 0;
            Vehicle.DistanceAlongEdge = 0.f;   

            const FRoadEdge* InitialEdge = RoadGraph->GetEdge(Vehicle.CurrentEdgeID);
            Vehicle.LogicalLocation = InitialEdge->StartLocation;
            Vehicle.LogicalRotation = (InitialEdge->EndLocation - InitialEdge->StartLocation).Rotation();   

            int32 Index = ActiveVehicles.Add(Vehicle);
            VehicleIDToIndex.Add(Vehicle.VehicleID, Index);

        }
    }
    UE_LOG(LogSimulation, Log, TEXT("VehicleSimulation: Spawned %d vehicles"), ActiveVehicles.Num());   
}

bool UVehicleSimulationSubsystem::GenerateRoute(FVehicleData& Vehicle, int32 StartNodeID, int32 TargetNodeID)
{
    URoadGraphSubsystem* RoadGraph = GetWorld()->GetSubsystem<URoadGraphSubsystem>();
    if (!RoadGraph) return false;

    // Breadth-First Search for valid edge sequence
    TArray<int32> Queue;
    TMap<int32, int32> CameFromNode;
    TMap<int32, int32> EdgeUsed;

    Queue.Add(StartNodeID);
    CameFromNode.Add(StartNodeID, -1);

    bool bFound = false;

    while (Queue.Num() > 0)
    {
        int32 Current = Queue[0];
        Queue.RemoveAt(0);

        if (Current == TargetNodeID)
        {
            bFound = true;
            break;
        }

        const FRoadNode* Node = RoadGraph->GetNode(Current);
        if (!Node) continue;

        for (int32 EdgeID : Node->ConnectedEdges)
        {
            const FRoadEdge* Edge = RoadGraph->GetEdge(EdgeID);
            if (!Edge) continue;

            // Only traverse forward (Start -> End)
            if (Edge->StartNodeID == Current && !CameFromNode.Contains(Edge->EndNodeID))
            {
                Queue.Add(Edge->EndNodeID);
                CameFromNode.Add(Edge->EndNodeID, Current);
                EdgeUsed.Add(Edge->EndNodeID, EdgeID);
            }
        }
    }

    if (!bFound) return false;

    // Reconstruct Path
    Vehicle.EdgeRoute.Empty();
    int32 TraceNode = TargetNodeID;

    while (TraceNode != StartNodeID)
    {
        Vehicle.EdgeRoute.Insert(EdgeUsed[TraceNode], 0);
        TraceNode = CameFromNode[TraceNode];
    }

    return Vehicle.EdgeRoute.Num() > 0;
}

void UVehicleSimulationSubsystem::ProcessTick(float FixedTickRate)
{
    if (ActiveVehicles.Num() == 0) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UNeedsSubsystem* Needs = World->GetSubsystem<UNeedsSubsystem>();

    for (FVehicleData& Vehicle : ActiveVehicles)
    {
        const FRoadEdge* Edge = nullptr;
        if (URoadGraphSubsystem* RoadGraph = World->GetSubsystem<URoadGraphSubsystem>())
        {
            Edge = RoadGraph->GetEdge(Vehicle.CurrentEdgeID);
        }

        if (!Edge) continue;

        float TargetSpeed = Vehicle.Speed;

         
        if (Needs)
        {
            for (const FAgentData& Agent : Needs->GetActiveAgents())
            {
                if (Agent.bInsideBuilding) continue;

                float DistSq = FVector::DistSquared(Vehicle.LogicalLocation, Agent.LogicalLocation);

                if (DistSq < 90000.f) // 300 units
                {
                    FVector ToAgent = (Agent.LogicalLocation - Vehicle.LogicalLocation).GetSafeNormal();
                    FVector Forward = Vehicle.LogicalRotation.Vector();

                    if (FVector::DotProduct(Forward, ToAgent) > 0.5f)
                    {
                        TargetSpeed = (DistSq < 22500.f) ? 0.f : TargetSpeed * 0.2f;   // hard stop within 150u, else crawl
                        break;
                    }
                }
            }
        }

        Vehicle.DistanceAlongEdge += TargetSpeed * FixedTickRate;

        if (Vehicle.DistanceAlongEdge >= Edge->Length)
        {
            HandleVehicleProgression(Vehicle, Edge);

            if (URoadGraphSubsystem* RoadGraph = World->GetSubsystem<URoadGraphSubsystem>())
            {
                Edge = RoadGraph->GetEdge(Vehicle.CurrentEdgeID);
            }

            if (!Edge) continue;

            Vehicle.DistanceAlongEdge = 0.f;
        }

        float Alpha = FMath::Clamp(Vehicle.DistanceAlongEdge / Edge->Length, 0.f, 1.f);

        Vehicle.LogicalLocation = FMath::Lerp(Edge->StartLocation, Edge->EndLocation, Alpha);
        Vehicle.LogicalRotation = (Edge->EndLocation - Edge->StartLocation).Rotation();
    }
}
// Failsafe routing logic. Forces deterministic roaming or random fallback edge selection to prevent vehicle deadlocks at topological dead-ends or unresolvable path states.
void UVehicleSimulationSubsystem::HandleVehicleProgression(FVehicleData& Vehicle, const FRoadEdge* CurrentEdge)
{
    Vehicle.CurrentRouteIndex++;  
    if (Vehicle.EdgeRoute.IsValidIndex(Vehicle.CurrentRouteIndex))  
    {
        Vehicle.CurrentEdgeID = Vehicle.EdgeRoute[Vehicle.CurrentRouteIndex];  
        Vehicle.DistanceAlongEdge = 0.f;  
    }
    else  
    {
        URoadGraphSubsystem* RoadGraph = GetWorld()->GetSubsystem<URoadGraphSubsystem>();  
        const TArray<FRoadEdge>& AllEdges = RoadGraph->GetAllEdges();  
        int32 StartNode = CurrentEdge->EndNodeID;   

        // ❌ TAXI STATE MACHINE INJECTION REMOVED ENTIRELY

        // DETERMINISTIC FALLBACK ROAMING
        int32 EndNode = AllEdges[(Vehicle.VehicleID * 7 + Vehicle.CurrentRouteIndex) % AllEdges.Num()].EndNodeID;  
        if (GenerateRoute(Vehicle, StartNode, EndNode))   
        {
            Vehicle.CurrentRouteIndex = 0;  
            Vehicle.CurrentEdgeID = Vehicle.EdgeRoute[0];   
            Vehicle.DistanceAlongEdge = 0.f;  
        }
        else    
        {
            const FRoadNode* Node = RoadGraph->GetNode(StartNode); 
            if (Node && Node->ConnectedEdges.Num() > 0)  
            {
                int32 EscapeEdge = Node->ConnectedEdges[Vehicle.VehicleID % Node->ConnectedEdges.Num()];  
                Vehicle.EdgeRoute.Empty();  
                Vehicle.EdgeRoute.Add(EscapeEdge);
                Vehicle.CurrentRouteIndex = 0; 
                Vehicle.CurrentEdgeID = EscapeEdge; 
                Vehicle.DistanceAlongEdge = 0.f;  
            }
            else 
            {
                int32 RandomFallbackEdge = AllEdges[Vehicle.VehicleID % AllEdges.Num()].EdgeID; 
                Vehicle.EdgeRoute.Empty(); 
                Vehicle.EdgeRoute.Add(RandomFallbackEdge); 
                Vehicle.CurrentRouteIndex = 0; 
                Vehicle.CurrentEdgeID = RandomFallbackEdge; 
                Vehicle.DistanceAlongEdge = 0.f; 
            }
        }
    }
}

void UVehicleSimulationSubsystem::Sim_DebugTransit()
{
    bDebugTransit = !bDebugTransit;
}

const FVehicleData* UVehicleSimulationSubsystem::GetVehicleData(int32 VehicleID) const
{
    if (const int32* Index = VehicleIDToIndex.Find(VehicleID))
    {
        if (ActiveVehicles.IsValidIndex(*Index))
        {
            return &ActiveVehicles[*Index];
        }
    }

    return nullptr;
}
bool UVehicleSimulationSubsystem::DispatchTaxi(int32 AgentID, FVector AgentLoc, int32 StartNode, int32 TargetNode)
{
    int32 BestTaxiID = -1;
    float BestDist = MAX_FLT;

    for (FVehicleData& Veh : ActiveVehicles)
    {
        // Driving acts as the "idle/available" state for wandering vehicles
        if (Veh.State == EVehicleState::Driving && Veh.DispatchedAgentID == -1)
        {
            float Dist = FVector::DistSquared(Veh.LogicalLocation, AgentLoc);
            if (Dist < BestDist)
            {
                BestDist = Dist;
                BestTaxiID = Veh.VehicleID;
            }
        }
    }

    if (BestTaxiID != -1)
    {
        // Fetch via existing index map
        int32 Index = VehicleIDToIndex[BestTaxiID];
        FVehicleData& Taxi = ActiveVehicles[Index];

        Taxi.DispatchedAgentID = AgentID;
        Taxi.TaxiDestinationNode = TargetNode;
         

        URoadPathfindingSubsystem* RoadPath = GetWorld()->GetSubsystem<URoadPathfindingSubsystem>();
        GenerateRoute(Taxi, RoadPath->GetNearestNode(Taxi.LogicalLocation), StartNode);

        Taxi.CurrentRouteIndex = 0;
        if (Taxi.EdgeRoute.Num() > 0)
        {
            Taxi.CurrentEdgeID = Taxi.EdgeRoute[0];
        }
        Taxi.DistanceAlongEdge = 0.f;

        return true;
    }
    return false;
}