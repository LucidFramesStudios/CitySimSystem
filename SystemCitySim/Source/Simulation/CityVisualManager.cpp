#include "Simulation/CityVisualManager.h"

ACityVisualManager::ACityVisualManager()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

void ACityVisualManager::ClearAll()
{
    for (auto& Pair : HISMMap)
    {
        if (Pair.Value)
        {
            Pair.Value->ClearInstances();
        }
    }
}

void ACityVisualManager::AddMeshInstance(UStaticMesh* Mesh, const FTransform& Transform)
{
    if (!Mesh) return;

    UHierarchicalInstancedStaticMeshComponent* HISM = nullptr;

    if (UHierarchicalInstancedStaticMeshComponent** Found = HISMMap.Find(Mesh))
    {
        HISM = *Found;
    }
    else
    {
        HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);

        HISM->RegisterComponent();
        HISM->SetStaticMesh(Mesh);
        HISM->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

        HISMMap.Add(Mesh, HISM);
    }

    HISM->AddInstance(Transform);
}