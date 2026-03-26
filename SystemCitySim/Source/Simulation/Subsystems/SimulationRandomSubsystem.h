#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SimulationRandomSubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API USimulationRandomSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    FRandomStream Random;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override
    {
        Super::Initialize(Collection);

        Random.Initialize(777777);
    }

    float Rand()
    {
        return Random.FRand();
    }

    float RandRange(float A, float B)
    {
        return Random.FRandRange(A, B);
    }

    int32 RandInt(int32 A, int32 B)
    {
        return Random.RandRange(A, B);
    }
};