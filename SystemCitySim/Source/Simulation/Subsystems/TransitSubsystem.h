#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TransitSubsystem.generated.h"

USTRUCT()
struct FBusStop
{
    GENERATED_BODY()

    UPROPERTY()
    int32 StopID = -1;

    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    UPROPERTY()
    int32 EdgeID = -1;

    
    UPROPERTY()
    float DistanceAlongEdge = 0.f;

    UPROPERTY()
    TArray<int32> WaitingAgents;
};


UCLASS()
class SIMULATIONSHOWCASE_API UTransitSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void ProcessTick(float DeltaTime);
    int32 RegisterBusStop(
        const FVector& Location,
        int32 EdgeID,
        float DistanceAlongEdge);

    TArray<int32>& GetWaitingAgents(int32 StopID);
    bool GetStopsOnEdge(int32 EdgeID, TArray<FBusStop>& OutStops) const;

    void BusArrivedAtStop(int32 VehicleID, int32 StopID);

    int32 FindNearestStop(const FVector& Location) const;

    FVector GetStopLocation(int32 StopID) const;

    int32 FindStopOnEdge(int32 EdgeID) const;

    void AddAgentToStopQueue(int32 StopID, int32 AgentID);

private:

    UPROPERTY()
    TArray<FBusStop> BusStops;
};
