#include "Simulation/Actors/BuildingActor.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "Simulation/Subsystems/RoadPathfindingSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Subsystems/RoadGraphSubsystem.h"
ABuildingActor::ABuildingActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    SmartObjectComp = CreateDefaultSubobject<USmartObjectComponent>(TEXT("SmartObjectComp"));
}

 
void ABuildingActor::SetupBuilding(FGameplayTag InType, int32 InMaxUsers)
{
    if (!SmartObjectComp) return;

    SmartObjectComp->ObjectType = InType;
    SmartObjectComp->MaxUsers = InMaxUsers;

    FVector Origin;
    FVector Extent;
    GetActorBounds(false, Origin, Extent);

    FVector Entry;

   
    if (MeshComponent && MeshComponent->DoesSocketExist(TEXT("EntrySocket")))
    {
        Entry = MeshComponent->GetSocketLocation(TEXT("EntrySocket"));
    }
    else
    {
        float Offset = FMath::Max(Extent.X, Extent.Y) + 100.f;
        Entry = Origin + GetActorForwardVector() * Offset;
    }
    if (Entry.ContainsNaN() || Entry.IsNearlyZero())
    {
        UE_LOG(LogSimulation, Error, TEXT("Invalid EntryLocation on %s"), *GetName());
        Entry = GetActorLocation();
    }
    FVector Inside = Origin;

  
    SmartObjectComp->EntryLocation = Entry;
    SmartObjectComp->InsideLocation = Inside;

        

    SmartObjectComp->InitializeSlots();

    if (UInteractionSubsystem* Sub = GetWorld()->GetSubsystem<UInteractionSubsystem>())
    {
        Sub->RegisterSmartObject(SmartObjectComp);
    }
}

void ABuildingActor::BeginPlay()
{
    Super::BeginPlay();
    if (MeshComponent && MeshComponent->GetStaticMesh())
    {
        DynamicMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
    }

    if (UWorld* World = GetWorld())
    {
        if (UWorldSimulationSubsystem* SimSub = World->GetSubsystem<UWorldSimulationSubsystem>())
        {
            SimSub->OnTimeChanged.AddDynamic(this, &ABuildingActor::OnTimeChanged);
        }
    }
}

void ABuildingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UWorldSimulationSubsystem* SimSub = World->GetSubsystem<UWorldSimulationSubsystem>())
        {
            SimSub->OnTimeChanged.RemoveDynamic(this, &ABuildingActor::OnTimeChanged);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void ABuildingActor::OnTimeChanged(float NewTime)
{
    if (DynamicMat)
    {
        bool bIsNight = (NewTime >= 19.0f || NewTime < 6.0f);
        DynamicMat->SetScalarParameterValue(FName("EmissionStrength"), bIsNight ? 10.0f : 0.0f);
    }
}

