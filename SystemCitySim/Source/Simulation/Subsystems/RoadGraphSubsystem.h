    #pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Components/SplineComponent.h"
#include "RoadGraphSubsystem.generated.h"

USTRUCT()
struct FRoadNode
{
    GENERATED_BODY()

    UPROPERTY()
    int32 NodeID = -1;

    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    UPROPERTY()
    TArray<int32> ConnectedEdges;
};

USTRUCT()
struct FRoadEdge
{
    GENERATED_BODY()

    UPROPERTY()
    int32 EdgeID = -1; 

    UPROPERTY()
    int32 StartNodeID = -1; 

    UPROPERTY()
    int32 EndNodeID = -1; 

    UPROPERTY()
    float Length = 0.f; 

    
    UPROPERTY()
    FVector StartLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector EndLocation = FVector::ZeroVector;
};

UCLASS()
class SIMULATIONSHOWCASE_API URoadGraphSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    bool ValidateGraphConnectivity();
    int32 AddNode(const FVector& Location);

    
    int32 AddEdge(
        int32 StartNode,
        int32 EndNode,
        FVector StartLoc,
        FVector EndLoc,
        float InLength);

    const FRoadNode* GetNode(int32 NodeID) const;
    const FRoadEdge* GetEdge(int32 EdgeID) const;

    const TArray<FRoadEdge>& GetAllEdges() const
    {
        return Edges;
    }

private:

    UPROPERTY()
    TArray<FRoadNode> Nodes;

    UPROPERTY()
    TArray<FRoadEdge> Edges;

    UPROPERTY()
    TMap<uint64, int32> NodeLocationHash;
};