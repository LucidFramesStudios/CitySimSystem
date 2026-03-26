#include "StreetLightActor.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"

AStreetLightActor::AStreetLightActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    LightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightComponent"));
    LightComponent->SetupAttachment(RootComponent);
    LightComponent->SetIntensity(0.f);
}

void AStreetLightActor::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UWorldSimulationSubsystem* SimSub = World->GetSubsystem<UWorldSimulationSubsystem>())
        {
            
            SimSub->OnTimeChanged.AddDynamic(this, &AStreetLightActor::OnTimeChanged);
        }
    }
}

void AStreetLightActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UWorldSimulationSubsystem* SimSub = World->GetSubsystem<UWorldSimulationSubsystem>())
        {
            SimSub->OnTimeChanged.RemoveDynamic(this, &AStreetLightActor::OnTimeChanged);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AStreetLightActor::OnTimeChanged(float NewTime)
{
    
    bool bIsNight = (NewTime >= 19.0f || NewTime < 6.0f);
    LightComponent->SetIntensity(bIsNight ? 5000.f : 0.f);
}