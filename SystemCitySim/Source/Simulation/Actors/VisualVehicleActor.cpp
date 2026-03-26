#include "Simulation/Actors/VisualVehicleActor.h"
#include "Simulation/Subsystems/VehicleSimulationSubsystem.h"

AVisualVehicleActor::AVisualVehicleActor()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
}

void AVisualVehicleActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SyncToSimulation(DeltaTime);
    DrawVehicleDebug();
}

void AVisualVehicleActor::SyncToSimulation(float DeltaTime)
{
    if (!GetWorld()) return;

    UVehicleSimulationSubsystem* VehSub =
        GetWorld()->GetSubsystem<UVehicleSimulationSubsystem>();

    if (!VehSub || AssignedVehicleID == -1) return;

    const FVehicleData* Veh =
        VehSub->GetVehicleData(AssignedVehicleID);

    if (!Veh) return;

    //  HARD SET (NO SMOOTHING)
    SetActorLocation(Veh->LogicalLocation);
    SetActorRotation(Veh->LogicalRotation);

     
}
void AVisualVehicleActor::DrawVehicleDebug()
{
    if (!ADebugHUD::bDebugVehicles) return;

    DrawDebugString(
        GetWorld(),
        GetActorLocation() + FVector(0, 0, 200),
        FString::Printf(TEXT("Veh %d"), AssignedVehicleID),
        nullptr,
        FColor::Orange,
        0.f,
        true
    );
}
