#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectMacros.h"
#include "WorldSimulationSubsystem.generated.h"

 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeChangedSignature, float, NewTime);

UCLASS()
class SIMULATIONSHOWCASE_API UWorldSimulationSubsystem :
    public UWorldSubsystem,
    public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override
    {
        if (!GetWorld()) return false;
        return GetWorld()->HasBegunPlay();
    }
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldSimulationSubsystem, STATGROUP_Tickables);
    }
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    
    float GetLastSimulationStepMS() const
    {
        return LastSimulationStepMS;
    }

    float GetAverageSimulationStepMS() const
    {
        return AverageSimulationStepMS;
    }
    

    UFUNCTION(BlueprintCallable, Category = "Simulation")
    float GetWorldTime() const { return WorldTime; }

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override
    {
        return true;
    }

    //-----------------------------------------
    // Simulation time
    //-----------------------------------------
 
    UPROPERTY(BlueprintReadOnly, Category = "Simulation|Weather")
    bool bIsRaining = false;

    UFUNCTION(BlueprintCallable, Category = "Simulation|Weather")
    void SetRain(bool bRainActive);
    UPROPERTY(BlueprintReadOnly, Category = "Simulation")
    float WorldTime = 8.0f; // START AT 8 AM

    UPROPERTY(EditAnywhere, Category = "Simulation|Time")
    float DayDurationSeconds = 4800.f; // 40 minutes real time = 24 hours

    UPROPERTY(EditAnywhere, Category = "Simulation|Time")
    float TimeScale = 1.0f; // multiplier (keep 1 for now)

    //-----------------------------------------

    UFUNCTION(Exec)
    void DumpSimulationState();

    //-----------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Simulation|Events")
    FOnTimeChangedSignature OnTimeChanged;

private:
    float LastSimulationStepMS = 0.f;

    float AverageSimulationStepMS = 0.f;

    int32 StepSampleCount = 0;
    float Accumulator = 0.f;

    const float FixedTickRate = 0.1f;

    void ExecuteSimulationStep();
};