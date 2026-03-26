#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CityEventSubsystem.generated.h"

UENUM(BlueprintType)
enum class ECityEvent : uint8
{
    None,
    RushHour,
    Rain,
    Festival,
    PowerOutage
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityEventChanged, ECityEvent, NewEvent);

UCLASS()
class SIMULATIONSHOWCASE_API UCityEventSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void ProcessTick(float WorldTime);

    UPROPERTY(BlueprintAssignable, Category = "Simulation|Events")
    FOnCityEventChanged OnCityEventChanged;

    ECityEvent GetCurrentEvent() const { return ActiveEvent; }

    UFUNCTION(BlueprintCallable, Category = "Simulation|Events")
    void ForceEvent(ECityEvent NewEvent, float DurationInHours = 1.0f);


private:
    ECityEvent ActiveEvent = ECityEvent::None;
    float EventEndTime = 0.f;

    void TriggerRandomEvent(float CurrentWorldTime);
    void EndCurrentEvent();
};