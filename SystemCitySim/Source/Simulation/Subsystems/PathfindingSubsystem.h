#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NavigationSystem.h"
#include "PathfindingSubsystem.generated.h"

struct FPathRequest
{
    int32 AgentID;
    int32 StartNode;
    int32 EndNode;
};

struct FPathResult
{
    int32 AgentID;
    TArray<FVector> PathPoints;
    bool bSuccess;
};

UCLASS()
class SIMULATIONSHOWCASE_API UPathfindingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void RequestPath(int32 AgentID, int32 StartNode, int32 EndNode);
    void ProcessResolvedPaths();

private:
    void OnPathResolved(uint32 PathID, ENavigationQueryResult::Type Result, FNavPathSharedPtr NavPointer, int32 AgentID);

    TArray<FPathRequest> PendingRequests;
    /** Async results buffer */
    TArray<FPathResult> ResolvedPaths;
    FCriticalSection PathMutex; 
};