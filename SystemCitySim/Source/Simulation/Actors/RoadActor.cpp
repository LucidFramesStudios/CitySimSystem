
#include "Simulation/Actors/RoadActor.h"

ARoadActor::ARoadActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RoadMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoadMeshComponent"));
    RoadMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RoadMeshComponent->SetCollisionObjectType(ECC_WorldStatic);
    RoadMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    RootComponent = RoadMeshComponent;
    Tags.Add(FName("Road"));
}

void ARoadActor::BuildRoad(FVector StartPos, FVector EndPos)
{
    if (!RoadMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("RoadMeshComponent NULL"));
        return;
    }

    if (!RoadMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("RoadMesh NOT ASSIGNED"));
        return;
    }

    RoadMeshComponent->SetStaticMesh(RoadMesh);

    
    StartPos.Z = 0.f;
    EndPos.Z = 0.f;

    FVector Direction = EndPos - StartPos;
    float Length = Direction.Size();
    if (Length <= 0.f) return;
    Direction.Normalize();

    
    FRotator Rotation = Direction.Rotation();
    FVector Center = StartPos + (Direction * (Length * 0.5f));
    SetActorLocationAndRotation(Center, Rotation);

    
    FBoxSphereBounds Bounds = RoadMesh->GetBounds();
    float MeshLength = Bounds.BoxExtent.X * 2.f; 

    if (MeshLength > 0.f)
    {
        RoadMeshComponent->SetWorldScale3D(FVector(Length / MeshLength, 1.0f, 1.0f));
    }
}