#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Simulation/Data/SimulationData.h"
#include "SmartObjectComponent.generated.h"



UCLASS(ClassGroup = (Simulation), meta = (BlueprintSpawnableComponent))
class SIMULATIONSHOWCASE_API USmartObjectComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    TArray<FInteractionSlot> Slots;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector EntryLocation;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector InsideLocation;
    UPROPERTY(EditAnywhere)
    FGameplayTag ObjectType;
    UPROPERTY(EditAnywhere)
    int32 ObjectID = -1;

    UPROPERTY(EditAnywhere)
    TArray<UInteractionDefinition*> AvailableInteractions;

    UPROPERTY(EditAnywhere)
    int32 MaxUsers = 3;

    UPROPERTY()
    int32 CurrentUsers = 0;
    virtual void BeginPlay() override;
    void InitializeSlots();

protected:

    
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};