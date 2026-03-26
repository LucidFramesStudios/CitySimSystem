#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoadPathfindingSubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API URoadPathfindingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    TArray<FVector> ComputePath(int32 StartNode, int32 TargetNode);

    int32 GetNearestNode(const FVector& Location) const;

private:

    TArray<int32> OpenSet;

    TMap<int32, int32> CameFrom;

    TMap<int32, float> GScore;

    TMap<int32, float> FScore;
};