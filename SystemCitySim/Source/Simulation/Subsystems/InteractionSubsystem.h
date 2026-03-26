#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/Components/SmartObjectComponent.h"
#include "Simulation/Data/SimulationData.h"
#include "InteractionSubsystem.generated.h"

class USmartObjectComponent;    
USTRUCT()
struct FSpatialCell
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> ObjectIDs;

    UPROPERTY()
    int32 AgentCount = 0; 
};

USTRUCT()
struct FSmartObjectEntry
{
    GENERATED_BODY()

    UPROPERTY()
    int32 ObjectID = -1;
     
    FVector InsideLocation;
    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    UPROPERTY()
    USmartObjectComponent* Component = nullptr;

    UPROPERTY()
    bool bReserved = false;

    UPROPERTY()
    FVector EntryLocation;
};

UCLASS()
class SIMULATIONSHOWCASE_API UInteractionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    FVector GetInsideLocation(int32 ObjectID) const;
    void ProcessTick(float FixedTickRate);

    void RegisterSmartObject(USmartObjectComponent* Object);

    void UnregisterSmartObject(USmartObjectComponent* Object);

    FInteractionSlot* ReserveSlot(int32 ObjectID, int32 AgentID);
    FVector GetObjectLocation(int32 ObjectID) const;
    void ReleaseSlot(int32 ObjectID, int32 AgentID);

    const TArray<FSmartObjectEntry>& GetAvailableObjects() const
    {
        return SmartObjects;
    }

    FSmartObjectEntry* GetSmartObjectByID(int32 ObjectID);

public:
    const USmartObjectComponent* GetSmartObject(int32 ObjectID) const;

    UInteractionDefinition* GetInteractionDefinition(FGameplayTag Tag);
    void UpdateAgentDensity();
    
    void QueryNearbyObjects(
        const FVector& Location,
        TArray<FSmartObjectEntry*>& OutObjects);

    UFUNCTION(Exec)
    void Sim_DebugSpatialGrid();

    void ResolveTimerCompletion(FAgentData& Agent);


    int32 FindBestObjectForGoal(
        FGameplayTag Goal,
        const FVector& AgentLocation,
        const TSet<int32>& FailedTargets,
        int32 CurrentObjectID
    ) const;

    FVector GetObjectEntryLocation(int32 ObjectID) const;
    void ProcessAgentInteraction(FAgentData& Agent, float DeltaTime);
protected:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:

    
    UPROPERTY()
    TArray<FSmartObjectEntry> SmartObjects;

    
    UPROPERTY()
    TArray<USmartObjectComponent*> RegisteredObjects;
    
    UPROPERTY()
    TMap<int32, int32> ObjectIDToIndex;

    
    TArray<FIntVector> NeighborOffsets;

    const float CellSize = 400.f;

    

    void RebuildIndexMap();

    

    void ApplyInteractionEffects(FAgentData& Agent);

public:
    FIntVector GetCellCoords(const FVector& Location) const;

    
    UPROPERTY()
    TMap<FIntVector, FSpatialCell> SpatialGrid;
    UPROPERTY()
    TMap<FGameplayTag, UInteractionDefinition*> RegisteredInteractionDefinitions;

    UPROPERTY(EditAnywhere, Category = "Simulation")
    
    TArray<UInteractionDefinition*> InteractionDefinitions;



};