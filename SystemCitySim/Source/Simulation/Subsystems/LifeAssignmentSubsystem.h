#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LifeAssignmentSubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API ULifeAssignmentSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    
    void ExecuteLifeAssignments();

private:
    void AssignFacilities(class UInteractionSubsystem* IntSub, class UNeedsSubsystem* NeedsSub, class USimulationRandomSubsystem* RandSub);
    void GenerateFriendNetworks(class UNeedsSubsystem* NeedsSub, class USimulationRandomSubsystem* RandSub);
};