#include "Simulation/Subsystems/TransitSubsystem.h"
#include "Simulation/Subsystems/RoadGraphSubsystem.h"

int32 UTransitSubsystem::RegisterBusStop(
    const FVector& Location,
    int32 EdgeID,
    float DistanceAlongEdge)
{
    FBusStop Stop;

    Stop.StopID = BusStops.Num();
    Stop.Location = Location;
    Stop.EdgeID = EdgeID;
    Stop.DistanceAlongEdge = DistanceAlongEdge;

    BusStops.Add(Stop);

    return Stop.StopID;
}


TArray<int32>& UTransitSubsystem::GetWaitingAgents(int32 StopID)
{
    return BusStops[StopID].WaitingAgents;
}

int32 UTransitSubsystem::FindNearestStop(
    const FVector& Location) const
{
    float BestDist = FLT_MAX;
    int32 BestStop = -1;

    for (const FBusStop& Stop : BusStops)
    {
        float Dist =
            FVector::DistSquared(Location, Stop.Location);

        if (Dist < BestDist)
        {
            BestDist = Dist;
            BestStop = Stop.StopID;
        }
    }

    return BestStop;
}

FVector UTransitSubsystem::GetStopLocation(int32 StopID) const
{
    if (BusStops.IsValidIndex(StopID))
    {
        return BusStops[StopID].Location;
    }

    return FVector::ZeroVector;
}

int32 UTransitSubsystem::FindStopOnEdge(int32 EdgeID) const
{
    for (const FBusStop& Stop : BusStops)
    {
        if (Stop.EdgeID == EdgeID)
        {
            return Stop.StopID;
        }
    }

    return -1;
}
bool UTransitSubsystem::GetStopsOnEdge(
    int32 EdgeID,
    TArray<FBusStop>& OutStops) const
{
    OutStops.Reset();

    for (const FBusStop& Stop : BusStops)
    {
        if (Stop.EdgeID == EdgeID)
        {
            OutStops.Add(Stop);
        }
    }

    return OutStops.Num() > 0;
}

void UTransitSubsystem::BusArrivedAtStop(
    int32 VehicleID,
    int32 StopID)
{
    
    

    UE_LOG(LogTemp, Verbose,
        TEXT("Bus %d arrived at stop %d"),
        VehicleID,
        StopID);
}
void UTransitSubsystem::AddAgentToStopQueue(int32 StopID, int32 AgentID)
{
    if (BusStops.IsValidIndex(StopID)) 
    {
        BusStops[StopID].WaitingAgents.AddUnique(AgentID); 
    }
}
void UTransitSubsystem::ProcessTick(float DeltaTime)
{
    
    
    

    
    
    
    

    

    

    
    
    
    

    
    
    
    
    

    

    
    
    
}