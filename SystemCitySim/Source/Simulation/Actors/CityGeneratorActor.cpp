#include "Simulation/Actors/CityGeneratorActor.h"
#include "Simulation/Actors/RoadActor.h"
#include "Simulation/Actors/BuildingActor.h"
#include "Simulation/Subsystems/RoadGraphSubsystem.h"
#include "Simulation/Subsystems/TransitSubsystem.h"
#include "Templates/UnrealTemplate.h"
#include "Simulation/Logging/SimulationLog.h"
#include "CollisionQueryParams.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Simulation/Data/SimulationData.h"
#include "Simulation/CityVisualManager.h"
#include "Simulation/SimulationTags.h"

ACityGeneratorActor::ACityGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

 

void ACityGeneratorActor::BeginPlay()
{
    Super::BeginPlay();
    FSimTags::Init();
    GenerateCity();
}

void ACityGeneratorActor::GenerateCityEditor()
{
    GenerateCity();
}



// Deterministic generation pipeline. Validates graph connectivity early to prevent invalid topological states before actor instantiation.

void ACityGeneratorActor::GenerateCity()
{
    InitializeGenerator();

    GenerateHubs();

    GenerateBlocksFromGrid();

    GenerateGridRoads();

    URoadGraphSubsystem* RoadGraph =
        GetWorld()->GetSubsystem<URoadGraphSubsystem>();

    if (RoadGraph)
    {
        if (!RoadGraph->ValidateGraphConnectivity())
        {
            UE_LOG(LogSimulation, Error,
                TEXT("CITY GENERATION FAILED: Graph disconnected"));
            return;
        }
    }

    FSimTags::Init();

    
    TArray<FCityParcel> Parcels;
    for (const FCityBlock& Block : CityBlocks)
    {
        Parcels.Append(SubdivideBlockIntoParcels(Block));
    }

    
    TArray<FCityCluster> Clusters = ClusterParcels(Parcels);

    
    SpawnBuildingsFromClusters(Clusters);

    
    SpawnBuildingsFromParcels(Parcels);
}

// O(N^2) agglomerative clustering heuristic. Merges adjacent parcels of identical zoning to reduce total actor count and generate cohesive district blocks.
TArray<FCityCluster> ACityGeneratorActor::ClusterParcels(const TArray<FCityParcel>& Parcels)
{
    TArray<FCityCluster> Clusters;
    if (Parcels.IsEmpty()) return Clusters;

    TArray<bool> Used;
    Used.Init(false, Parcels.Num());

    for (int32 i = 0; i < Parcels.Num(); i++)
    {
        if (Used[i]) continue;

        
        if (SimRandomStream.FRand() < 0.1f)
        {
            Used[i] = true;
            continue;
        }

        FCityCluster Cluster;
        Cluster.ZoneType = Parcels[i].ZoneType;

        TArray<int32> Indices;
        Indices.Add(i);
        Used[i] = true;

        int32 MaxClusterSize = 1;

        if (Cluster.ZoneType == FSimTags::Type_Office)
            MaxClusterSize = SimRandomStream.RandRange(2, 4); 
        else if (Cluster.ZoneType == FSimTags::Type_Cafe || Cluster.ZoneType == FSimTags::Type_Shop)
            MaxClusterSize = SimRandomStream.RandRange(1, 3);
        else
            MaxClusterSize = SimRandomStream.RandRange(1, 2);

        FBox ClusterBox(
            Parcels[i].Transform.GetLocation() - Parcels[i].Dimensions * 0.5f,
            Parcels[i].Transform.GetLocation() + Parcels[i].Dimensions * 0.5f
        );

        while (Indices.Num() < MaxClusterSize)
        {
            int32 Best = -1;
            float BestDist = MAX_FLT;

            FVector ClusterCenter = ClusterBox.GetCenter();

            for (int32 j = 0; j < Parcels.Num(); j++)
            {
                if (Used[j]) continue;
                if (Parcels[j].ZoneType != Cluster.ZoneType) continue;
                if (Parcels[j].BlockID != Parcels[i].BlockID) continue;

                FVector PLoc = Parcels[j].Transform.GetLocation();

                
                FVector Dir = (PLoc - ClusterCenter).GetSafeNormal();
                if (!(FMath::Abs(Dir.X) > 0.7f || FMath::Abs(Dir.Y) > 0.7f))
                    continue;

                FBox ParcelBox(
                    PLoc - Parcels[j].Dimensions * 0.5f,
                    PLoc + Parcels[j].Dimensions * 0.5f
                );

                FBox Expanded = ClusterBox.ExpandBy(ParcelInset * 0.5f);

                if (Expanded.Intersect(ParcelBox))
                {
                    float Dist = FVector::DistSquared(ClusterCenter, PLoc);
                    if (Dist < BestDist)
                    {
                        BestDist = Dist;
                        Best = j;
                    }
                }
            }

            if (Best != -1)
            {
                Used[Best] = true;
                Indices.Add(Best);

                FBox AddBox(
                    Parcels[Best].Transform.GetLocation() - Parcels[Best].Dimensions * 0.5f,
                    Parcels[Best].Transform.GetLocation() + Parcels[Best].Dimensions * 0.5f
                );

                ClusterBox += AddBox;
            }
            else break;
        }

        Cluster.CenterTransform = FTransform(ClusterBox.GetCenter());
        Cluster.CombinedDimensions = ClusterBox.GetSize();
        Cluster.ParcelCount = Indices.Num();

        Clusters.Add(Cluster);
    }

    return Clusters;
}




void ACityGeneratorActor::InitializeGenerator()
{
    SimRandomStream.Initialize(RandomSeed);

    CityHubs.Empty();
    CityBlocks.Empty();

    if (VisualManager)
    {
        VisualManager->ClearAll();
    }

    for (ARoadActor* Road : GeneratedRoads)
    {
        if (IsValid(Road))
        {
            Road->Destroy();
        }
    }

    GeneratedRoads.Empty();

    for (ABuildingActor* Building : GeneratedBuildings)
    {
        if (IsValid(Building))
        {
            Building->Destroy();
        }
    }

    GeneratedBuildings.Empty();
}






void ACityGeneratorActor::GenerateHubs()
{
    CityHubs.Empty(); 

    
    FCityHub DowntownHub;
    DowntownHub.Location = GetActorLocation(); 
    DowntownHub.Radius = CityRadius * 0.4f; 
    DowntownHub.DominantZone = FSimTags::Type_Office; 
    DowntownHub.DensityMultiplier = 2.5f; 
    DowntownHub.HeightMultiplier = 4.0f; 
    CityHubs.Add(DowntownHub); 

    if (HubLandmarkMesh && VisualManager)
    {
        VisualManager->AddMeshInstance(HubLandmarkMesh, FTransform(DowntownHub.Location)); 
    }

    
    int32 ResidentialHubs = TargetHubCount / 2;  
    for (int32 i = 0; i < ResidentialHubs; ++i)
    {
        FCityHub ResHub;
        float Angle = (PI * 2.f / ResidentialHubs) * i; 
        float Dist = CityRadius * 0.75f; 

        ResHub.Location = GetActorLocation() + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f); 
        ResHub.Radius = CityRadius * 0.35f; 
        ResHub.DominantZone = FSimTags::Type_Home; 
        ResHub.DensityMultiplier = 0.8f; 
        ResHub.HeightMultiplier = 0.6f; 
        CityHubs.Add(ResHub); 
    }

    
    int32 LeisureHubs = TargetHubCount - ResidentialHubs - 1; 
    for (int32 i = 0; i < LeisureHubs; ++i)
    {
        FCityHub LeisureHub;
        float Angle = SimRandomStream.FRandRange(0.f, PI * 2.f); 
        float Dist = SimRandomStream.FRandRange(CityRadius * 0.3f, CityRadius * 0.6f); 

        LeisureHub.Location = GetActorLocation() + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f); 
        LeisureHub.Radius = SimRandomStream.FRandRange(2000.f, 4000.f); 

        float Roll = SimRandomStream.FRand();
        if (i == 0) LeisureHub.DominantZone = FSimTags::Type_Cafe;   // guarantee at least one cafe hub (roll still consumed for determinism)
        else if (Roll < 0.4f) LeisureHub.DominantZone = FSimTags::Type_Park;
        else if (Roll < 0.8f) LeisureHub.DominantZone = FSimTags::Type_Cafe;
        else LeisureHub.DominantZone = FSimTags::Type_Shop;

        LeisureHub.DensityMultiplier = 1.0f; 
        LeisureHub.HeightMultiplier = 0.8f; 
        CityHubs.Add(LeisureHub); 
    }

    
    FCityHub IndustrialHub;
    IndustrialHub.Location = GetActorLocation() + FVector(CityRadius * 0.5f, 0, 0);
    IndustrialHub.Radius = CityRadius * 0.25f;
    IndustrialHub.DominantZone = FSimTags::Type_Industrial;
    IndustrialHub.DensityMultiplier = 1.2f;
    IndustrialHub.HeightMultiplier = 1.5f;

    CityHubs.Add(IndustrialHub);

}

float ACityGeneratorActor::ComputeDensityAtLocation(
    const FVector& Location) const
{
    float MaxDensity = 0.f;

    for (const FCityHub& Hub : CityHubs)
    {
        float Dist =
            FVector::Dist2D(Location, Hub.Location);

        if (Dist <= Hub.Radius)
        {
            float Falloff =
                1.f - (Dist / Hub.Radius);

            float LocalDensity =
                Falloff * Hub.DensityMultiplier;

            MaxDensity =
                FMath::Max(MaxDensity, LocalDensity);
        }
    }

    return MaxDensity;
}















void ACityGeneratorActor::GenerateGridRoads()
{
    ClearRoads();
    SidewalkSegments.Empty();

    URoadGraphSubsystem* RoadGraph = GetWorld()->GetSubsystem<URoadGraphSubsystem>();
    UTransitSubsystem* Transit = GetWorld()->GetSubsystem<UTransitSubsystem>();
    if (!RoadGraph || !Transit) return;

    RoadCenters.Empty();

    FVector ActorLoc = GetActorLocation();
    TSet<uint64> ProcessedEdges;

    auto GetEdgeHash = [](const FVector& A, const FVector& B)->uint64
        {
            FIntVector IntA((int32)(A.X * 0.01f), (int32)(A.Y * 0.01f), 0);
            FIntVector IntB((int32)(B.X * 0.01f), (int32)(B.Y * 0.01f), 0);

            if (IntA.X > IntB.X || (IntA.X == IntB.X && IntA.Y > IntB.Y))
            {
                Swap(IntA, IntB);
            }

            uint64 HashA = GetTypeHash(IntA);
            uint64 HashB = GetTypeHash(IntB);

            return HashA ^ (HashB + 0x9e3779b97f4a7c15ULL + (HashA << 6) + (HashA >> 2));
        };

    for (const FCityBlock& Block : CityBlocks)
    {
        if (FVector::Dist2D(Block.Center, ActorLoc) > CityRadius)
            continue;

        FVector BL = Block.Center + FVector(-Block.Extents.X, -Block.Extents.Y, 0);
        FVector BR = Block.Center + FVector(Block.Extents.X, -Block.Extents.Y, 0);
        FVector TL = Block.Center + FVector(-Block.Extents.X, Block.Extents.Y, 0);
        FVector TR = Block.Center + FVector(Block.Extents.X, Block.Extents.Y, 0);

        struct FEdgeDef { FVector Start; FVector End; };
        FEdgeDef Edges[4] = { {BL,BR}, {BR,TR}, {TR,TL}, {TL,BL} };

        for (int i = 0; i < 4; i++)
        {
            FVector Start = Edges[i].Start;
            FVector End = Edges[i].End;

            uint64 Hash = GetEdgeHash(Start, End);

            if (ProcessedEdges.Contains(Hash))
                continue;

            ProcessedEdges.Add(Hash);

            
            FVector Center = (Start + End) * 0.5f;
            RoadCenters.Add(Center);

            
            

            
            FVector GraphStart = Start;
            FVector GraphEnd = End;

            GraphStart.Z = 0.f;
            GraphEnd.Z = 0.f;

            int32 NodeA = RoadGraph->AddNode(GraphStart);
            int32 NodeB = RoadGraph->AddNode(GraphEnd);

            float Length = FVector::Dist(GraphStart, GraphEnd);
            int32 EdgeID = RoadGraph->AddEdge(NodeA, NodeB, GraphStart, GraphEnd, Length);

            
            if (Length > 2000.f && SimRandomStream.FRand() > 0.6f)
            {
                FVector StopLoc = (GraphStart + GraphEnd) * 0.5f;
                Transit->RegisterBusStop(StopLoc, EdgeID, Length * 0.5f);
            }

            
            
        }
    }
}









void ACityGeneratorActor::GenerateBlocksFromGrid()
{
    CityBlocks.Empty();
    int32 GridLimit = FMath::CeilToInt(CityRadius / BlockSize);
    int32 ID = 0;
    FVector ActorLoc = GetActorLocation();

    for (int32 X = -GridLimit; X <= GridLimit; X++)
    {
        for (int32 Y = -GridLimit; Y <= GridLimit; Y++)
        {
            FVector Center = ActorLoc + FVector(X * BlockSize + (BlockSize * 0.5f), Y * BlockSize + (BlockSize * 0.5f), 0.f);
            if (FVector::Dist2D(Center, ActorLoc) > CityRadius) continue;

            FCityBlock Block;
            Block.BlockID = ID++;
            Block.Center = Center;
            Block.Extents = FVector(BlockSize * 0.5f, BlockSize * 0.5f, 0.f);

            const int32 ParkDensityN = 6;
            if ((Block.BlockID + RandomSeed) % ParkDensityN == 0)
            {
                Block.PrimaryZone = FSimTags::Type_Park;
            }
            else
            {
               Block.PrimaryZone = DetermineZoneForLocation(Center);

                
                if (Block.PrimaryZone == FSimTags::Type_Home)
                {
                    int32 Hash = (Block.BlockID * 73 + RandomSeed) % 100;

                    if (Hash < 15)
                    {
                        Block.PrimaryZone = (Block.BlockID % 2 == 0)
                            ? FSimTags::Type_Shop
                            : FSimTags::Type_Cafe;
                    }
                }
            }

            Block.Density = ComputeDensityAtLocation(Center);
            CityBlocks.Add(Block);
        }
    }
}

TArray<FCityParcel> ACityGeneratorActor::SubdivideBlockIntoParcels(const FCityBlock& Block)
{
    TArray<FCityParcel> Parcels;
    TArray<FBox> Queue;

    FVector InsetExtents = Block.Extents - FVector(ParcelInset, ParcelInset, 0.f);
    if ((InsetExtents.X * 2.f) < MinParcelSize || (InsetExtents.Y * 2.f) < MinParcelSize)
    {
        return Parcels;
    }

    Queue.Add(FBox(Block.Center - InsetExtents, Block.Center + InsetExtents));

    int32 ReadIndex = 0;
    const int32 MaxSubdivisions = 1000;
    const float ParcelSpacing = 150.f; 

    while (ReadIndex < Queue.Num() && ReadIndex < MaxSubdivisions)
    {
        FBox Box = Queue[ReadIndex++];
        FVector Size = Box.GetSize();
        bool bSplitX = Size.X > Size.Y;

        float AspectRatio = FMath::Max(Size.X, Size.Y) / FMath::Max(FMath::Min(Size.X, Size.Y), 1.f);

        if (AspectRatio > 2.0f || ((bSplitX && Size.X >= MinParcelSize * 2.f) || (!bSplitX && Size.Y >= MinParcelSize * 2.f)))
        {
            float Ratio = SimRandomStream.FRandRange(0.4f, 0.6f);
            FBox A = Box;
            FBox B = Box;

            if (bSplitX)
            {
                float Split = Box.Min.X + (Size.X * Ratio);
                A.Max.X = Split - ParcelSpacing;
                B.Min.X = Split + ParcelSpacing;
            }
            else
            {
                float Split = Box.Min.Y + (Size.Y * Ratio);
                A.Max.Y = Split - ParcelSpacing;
                B.Min.Y = Split + ParcelSpacing;
            }

            Queue.Add(A);
            Queue.Add(B);
        }
        else
        {
            FCityParcel Parcel;
            Parcel.Transform = FTransform(Box.GetCenter());
            Parcel.Dimensions = Box.GetSize();
            Parcel.BlockID = Block.BlockID;

            
            Parcel.ZoneType = Block.PrimaryZone;
            Parcels.Add(Parcel);
        }
    }

    return Parcels;
}





// Intersection validation. Prevents building footprint overlaps by checking distance against previously established road graph edge centers.
void ACityGeneratorActor::SpawnBuildingsFromClusters(const TArray<FCityCluster>& Clusters)
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (const FCityCluster& Cluster : Clusters)
    {
        FVector Location = Cluster.CenterTransform.GetLocation();
        FGameplayTag Zone = Cluster.ZoneType;

        if (!Zone.IsValid()) continue;

        FRotator BuildRot(0.f, SimRandomStream.RandRange(0, 3) * 90.f, 0.f);

        ABuildingActor* Building = GetWorld()->SpawnActor<ABuildingActor>(
            BuildingClass, Location, BuildRot, Params);

        if (!Building) continue;

        GeneratedBuildings.Add(Building);

        
        if (Zone == FSimTags::Type_Park)
        {
            if (ParkGroundMesh)
            {
                Building->MeshComponent->SetStaticMesh(ParkGroundMesh);

                FBoxSphereBounds Bounds = ParkGroundMesh->GetBounds();

                float ScaleX = Cluster.CombinedDimensions.X / FMath::Max(Bounds.BoxExtent.X * 2.f, 1.f);
                float ScaleY = Cluster.CombinedDimensions.Y / FMath::Max(Bounds.BoxExtent.Y * 2.f, 1.f);

                Building->SetActorScale3D(FVector(ScaleX, ScaleY, 1.0f));

                Building->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                Building->MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
            }

            Building->SetupBuilding(Zone, 10000);
            Building->SmartObjectComp->EntryLocation = Location;
            Building->SmartObjectComp->InsideLocation = Location;

            continue;
        }

        
        UStaticMesh* SelectedMesh = nullptr;

        if (FBuildingRecipe* Recipe = DistrictRecipes.Find(Zone))
        {
            if (Recipe->BodyMeshes.Num() > 0)
            {
                SelectedMesh = Recipe->BodyMeshes[
                    SimRandomStream.RandRange(0, Recipe->BodyMeshes.Num() - 1)];
            }
            else
            {
                SelectedMesh = Recipe->BaseMesh;
            }
        }

        if (!SelectedMesh && BuildingMeshes.Num() > 0)
        {
            SelectedMesh = BuildingMeshes[
                SimRandomStream.RandRange(0, BuildingMeshes.Num() - 1)];
        }

        if (!SelectedMesh)
        {
            Building->Destroy();
            continue;
        }

        Building->MeshComponent->SetStaticMesh(SelectedMesh);

        FBoxSphereBounds Bounds = SelectedMesh->GetBounds();

        float ScaleX = Cluster.CombinedDimensions.X / FMath::Max(Bounds.BoxExtent.X * 2.f, 1.f);
        float ScaleY = Cluster.CombinedDimensions.Y / FMath::Max(Bounds.BoxExtent.Y * 2.f, 1.f);

        float ScaleZ = SimRandomStream.FRandRange(1.5f, 5.0f);

        if (Zone == FSimTags::Type_Cafe)
        {
            ScaleX *= 0.6f;
            ScaleY *= 0.6f;
            ScaleZ = SimRandomStream.FRandRange(0.5f, 1.2f);
        }
        else if (Zone == FSimTags::Type_Office)
        {
            ScaleZ = SimRandomStream.FRandRange(3.0f, 8.0f);
        }
        else if (Zone == FSimTags::Type_Home)
        {
            ScaleZ = SimRandomStream.FRandRange(1.5f, 3.0f);
            ScaleX *= 0.7f;
            ScaleY *= 0.7f;
        }

        Building->SetActorScale3D(FVector(ScaleX, ScaleY, ScaleZ));

        int32 MaxUsers = (Zone == FSimTags::Type_Office)
            ? SimRandomStream.RandRange(40, 120)
            : SimRandomStream.RandRange(5, 25);

        Building->SetupBuilding(Zone, MaxUsers);

        Building->SmartObjectComp->EntryLocation = Location;
        Building->SmartObjectComp->InsideLocation = Location;
    }
}





ARoadActor* ACityGeneratorActor::SpawnRoadActor()
{
    FActorSpawnParameters Params;

    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    return GetWorld()->SpawnActor<ARoadActor>(
        RoadClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params);
}

#if WITH_EDITOR
void ACityGeneratorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    
}
#endif

FGameplayTag ACityGeneratorActor::DetermineZoneForLocation(const FVector& Location) const
{
    FGameplayTag BestZone = FSimTags::Type_Home;
    float HighestInfluence = -1.f;

    for (const FCityHub& Hub : CityHubs)
    {
        float Dist = FVector::Dist2D(Location, Hub.Location);
        if (Dist <= Hub.Radius)
        {
            float Influence = 1.f - (Dist / Hub.Radius);
            if (Influence > HighestInfluence)
            {
                HighestInfluence = Influence;
                BestZone = Hub.DominantZone;
            }
        }
    }
    return BestZone;
}

void ACityGeneratorActor::SpawnBuildingsFromParcels(const TArray<FCityParcel>& Parcels)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (const FCityParcel& Parcel : Parcels)
    {
        FVector Location = Parcel.Transform.GetLocation();
        FGameplayTag Zone = Parcel.ZoneType;

        bool bNearRoad = false;

        for (const FVector& RoadCenter : RoadCenters)
        {
            float Dist = FVector::Dist2D(Location, RoadCenter);

            if (Dist < 700.f)
            {
                bNearRoad = true;
                break;
            }
        }

        if (bNearRoad)
        {
            continue;
        }

        
        if (Zone == FSimTags::Type_Park)
        {
            ABuildingActor* ParkBuilding = GetWorld()->SpawnActor<ABuildingActor>(
                BuildingClass, Location, FRotator::ZeroRotator, SpawnParams
            );

            if (ParkBuilding)
            {
                GeneratedBuildings.Add(ParkBuilding);

                FBoxSphereBounds ParkBounds = FBoxSphereBounds(FVector::ZeroVector, FVector(500.f, 500.f, 100.f), 500.f);

                if (ParkGroundMesh)
                {
                    ParkBounds = ParkGroundMesh->GetBounds();
                    float ParkWidth = ParkBounds.BoxExtent.X * 2.f;
                    float ParkDepth = ParkBounds.BoxExtent.Y * 2.f;

                    
                    if (ParkWidth > Parcel.Dimensions.X || ParkDepth > Parcel.Dimensions.Y)
                    {
                        ParkBuilding->MeshComponent->SetStaticMesh(nullptr);
                    }
                    else
                    {
                        
                        ParkBuilding->MeshComponent->SetStaticMesh(ParkGroundMesh);
                        ParkBuilding->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

                        
                        ParkBuilding->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                        ParkBuilding->MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
                        ParkBuilding->MeshComponent->SetCanEverAffectNavigation(true);
                    }
                }
                else
                {
                    ParkBuilding->MeshComponent->SetStaticMesh(nullptr);
                }

                
                if (TreeMesh && VisualManager)
                {
                    int32 TreeCount = SimRandomStream.RandRange(20, 40);
                    for (int32 i = 0; i < TreeCount; i++)
                    {
                        float EdgeScaleX = (SimRandomStream.FRand() > 0.5f ? 1.f : -1.f) * SimRandomStream.FRandRange(0.05f, 0.95f);
                        float EdgeScaleY = (SimRandomStream.FRand() > 0.5f ? 1.f : -1.f) * SimRandomStream.FRandRange(0.05f, 0.95f);

                        FVector TreeLoc = Location + FVector(
                            ParkBounds.BoxExtent.X * EdgeScaleX,
                            ParkBounds.BoxExtent.Y * EdgeScaleY,
                            0.f
                        );

                        VisualManager->AddMeshInstance(TreeMesh, FTransform(TreeLoc));
                    }
                }

                int32 MaxUsers = 10000;
                ParkBuilding->SetupBuilding(Zone, MaxUsers);
                ParkBuilding->SmartObjectComp->EntryLocation = Location + FVector(Parcel.Dimensions.X * 0.45f, 0.f, 0.f);
                ParkBuilding->SmartObjectComp->InsideLocation = Location;
            }
            continue;
        }

        ABuildingActor* Building = GetWorld()->SpawnActor<ABuildingActor>(
            BuildingClass,
            Location,
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (!Building) continue;

        GeneratedBuildings.Add(Building);

        float BaseScaleZ = SimRandomStream.FRandRange(0.9f, 1.1f);
        float HubHeightMult = 1.0f;
        float DensityMult = 1.0f;

        for (const FCityHub& Hub : CityHubs)
        {
            if (FVector::Dist2D(Location, Hub.Location) <= Hub.Radius)
            {
                HubHeightMult = Hub.HeightMultiplier;
                DensityMult = Hub.DensityMultiplier;
                break;
            }
        }

        float FinalHeightZ = BaseScaleZ * HubHeightMult * DensityMult;

        UStaticMesh* SelectedMesh = nullptr;

        if (FBuildingRecipe* Recipe = DistrictRecipes.Find(Zone))
        {
            if (Recipe->BodyMeshes.Num() > 0)
            {
                SelectedMesh = Recipe->BodyMeshes[
                    SimRandomStream.RandRange(0, Recipe->BodyMeshes.Num() - 1)
                ];
            }
            else
            {
                SelectedMesh = Recipe->BaseMesh;
            }
        }
        else if (BuildingMeshes.Num() > 0)
        {
            SelectedMesh = BuildingMeshes[
                SimRandomStream.RandRange(0, BuildingMeshes.Num() - 1)
            ];
        }

        if (!SelectedMesh) continue;

        Building->MeshComponent->SetStaticMesh(SelectedMesh);

        if (TreeMesh && VisualManager && Zone != FSimTags::Type_Industrial)
        {
            if (SimRandomStream.FRand() > 0.5f)
            {
                int32 GlobalTreeCount = SimRandomStream.RandRange(1, 2);
                for (int32 i = 0; i < GlobalTreeCount; i++)
                {
                    float CornerX = (SimRandomStream.FRand() > 0.5f ? 1.f : -1.f) * SimRandomStream.FRandRange(0.45f, 0.49f);
                    float CornerY = (SimRandomStream.FRand() > 0.5f ? 1.f : -1.f) * SimRandomStream.FRandRange(0.45f, 0.49f);
                    FVector TreeLoc = Location + FVector(Parcel.Dimensions.X * CornerX, Parcel.Dimensions.Y * CornerY, 0.f);
                    VisualManager->AddMeshInstance(TreeMesh, FTransform(TreeLoc));
                }
            }
        }

        FBoxSphereBounds Bounds = SelectedMesh->GetBounds();
        float MeshWidth = Bounds.BoxExtent.X * 2.f;
        float MeshDepth = Bounds.BoxExtent.Y * 2.f;

        float UniformScale = FMath::Min(
            (Parcel.Dimensions.X * 0.8f) / MeshWidth,
            (Parcel.Dimensions.Y * 0.8f) / MeshDepth
        );

        UniformScale = FMath::Clamp(UniformScale, 0.4f, 2.5f);

        float ScaleX = UniformScale;
        float ScaleY = UniformScale;

        float FootprintFactor = FMath::Sqrt(UniformScale);

        FinalHeightZ = BaseScaleZ * FootprintFactor * HubHeightMult;
        FinalHeightZ = FMath::Clamp(FinalHeightZ, 0.5f, 6.0f);

        if (Zone == FSimTags::Type_Cafe)
        {
            ScaleX *= 0.6f;
            ScaleY *= 0.6f;
            FinalHeightZ = SimRandomStream.FRandRange(0.3f, 0.8f);
        }

        if (Zone == FSimTags::Type_Office)
        {
            FinalHeightZ = FMath::Max(2.5f, FinalHeightZ);
        }

        if (Zone == FSimTags::Type_Home)
        {
            FinalHeightZ = FMath::Clamp(FinalHeightZ, 1.0f, 1.2f);
            ScaleX *= 0.7f;
            ScaleY *= 0.7f;
        }

        Building->SetActorScale3D(FVector(
            ScaleX,
            ScaleY,
            FMath::Clamp(FinalHeightZ, 1.0f, 2.5f)
        ));

        int32 MaxUsers = (Zone == FSimTags::Type_Office)
            ? FMath::RoundToInt(80 * DensityMult)
            : SimRandomStream.RandRange(5, 15);

        if (Zone == FSimTags::Type_Cafe)
        {
            MaxUsers = SimRandomStream.RandRange(15, 40);
        }

        Building->SetupBuilding(Zone, MaxUsers);
    }
}

void ACityGeneratorActor::ClearRoads()
{
    for (ARoadActor* Road : SpawnedRoads)
    {
        if (IsValid(Road))
        {
            Road->Destroy();
        }
    }

    SpawnedRoads.Empty();
}

const TArray<FSidewalkSegment>& ACityGeneratorActor::GetSidewalkSegments() const
{
    return SidewalkSegments;
}