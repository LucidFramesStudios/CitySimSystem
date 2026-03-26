#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/BuildingRecipe.h"
#include "GameplayTagContainer.h"
#include "CityGeneratorActor.generated.h"

class ARoadActor;
class ABuildingActor;
class ACityVisualManager;
class USplineComponent;





USTRUCT(BlueprintType)
struct FCityBlock
{
    GENERATED_BODY()

    UPROPERTY()
    int32 BlockID = -1;

    UPROPERTY()
    FVector Center = FVector::ZeroVector;

    UPROPERTY()
    FVector Extents = FVector::ZeroVector;

    UPROPERTY()
    FGameplayTag PrimaryZone;

    UPROPERTY()
    float Density = 1.0f;
};


USTRUCT(BlueprintType)
struct FCityCluster
{
    GENERATED_BODY()

    UPROPERTY()
    FTransform CenterTransform;

    UPROPERTY()
    FVector CombinedDimensions;

    UPROPERTY()
    FGameplayTag ZoneType;

    UPROPERTY()
    int32 ParcelCount = 0;
};




USTRUCT(BlueprintType)
struct FCityParcel
{
    GENERATED_BODY()

    UPROPERTY()
    FTransform Transform;

    UPROPERTY()
    FVector Dimensions;

    UPROPERTY()
    FGameplayTag ZoneType;

    UPROPERTY()
    int32 BlockID = -1;
};
USTRUCT()
struct FSidewalkSegment
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Start;

    UPROPERTY()
    FVector End;

    UPROPERTY()
    FVector Dir;

    UPROPERTY()
    float Length;

    UPROPERTY()
    float Width;

    UPROPERTY()
    TArray<int32> ConnectedSegments;


    FSidewalkSegment() {}

    FSidewalkSegment(const FVector& A, const FVector& B, float InWidth)
    {
        Start = A;
        End = B;
        Width = InWidth;

        FVector Delta = B - A;
        Length = Delta.Size();
        Dir = Delta.GetSafeNormal();
    }
};




USTRUCT(BlueprintType)
struct FCityHub
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere)
    float Radius = 4000.f;

    UPROPERTY(EditAnywhere)
    float DensityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere)
    float HeightMultiplier = 1.0f;

    UPROPERTY(EditAnywhere)
    FGameplayTag DominantZone;
};





UCLASS()
class SIMULATIONSHOWCASE_API ACityGeneratorActor : public AActor
{
    GENERATED_BODY()

public:

    ACityGeneratorActor();
    const TArray<FSidewalkSegment>& GetSidewalkSegments() const;
    FGameplayTag DetermineZoneForLocation(const FVector& Location) const;

    float ComputeDensityAtLocation(
        const FVector& Location) const;

    virtual void BeginPlay() override;

    UFUNCTION(CallInEditor, Category = "City Generation")
    void GenerateCityEditor();

    
    
    

    UPROPERTY(EditAnywhere, Category = "City|Parcels")
    float BlockSize = 3000.f;

    UPROPERTY(EditAnywhere, Category = "City|Parcels")
    float MinParcelSize = 1200.f;

    UPROPERTY(EditAnywhere, Category = "City|Parcels")
    float ParcelInset = 600.f;
    UPROPERTY(EditAnywhere, Category = "City|Props") UStaticMesh* ParkGroundMesh;
    
    
    

    UPROPERTY(EditAnywhere, Category = "City|Visuals")
    ACityVisualManager* VisualManager;

    UPROPERTY(EditAnywhere, Category = "City|Visuals")
    UStaticMesh* DebugBuildingMesh;

    UPROPERTY(EditAnywhere, Category = "City|Visuals")
    TArray<UStaticMesh*> BuildingMeshes;

    UPROPERTY(EditAnywhere, Category = "City|Visuals")
    TMap<FGameplayTag, FBuildingRecipe> DistrictRecipes;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    
    
    

    UPROPERTY(EditAnywhere, Category = "City|Props")
    UStaticMesh* StreetLightMesh;

    UPROPERTY(EditAnywhere, Category = "City|Props")
    UStaticMesh* TreeMesh;

    UPROPERTY(EditAnywhere, Category = "City|Props")
    UStaticMesh* BuildingBaseMesh;

    UPROPERTY(EditAnywhere, Category = "City|Props")
    UStaticMesh* HubLandmarkMesh;

    UPROPERTY(EditAnywhere, Category = "City|Props")
    TArray<UStaticMesh*> OfficeProps;

    UPROPERTY(EditAnywhere, Category = "City|Props")
    TArray<UStaticMesh*> ResidentialProps;

    UPROPERTY(EditAnywhere, Category = "City|Props")
    TArray<UStaticMesh*> CommercialProps;

    
    
    

    UPROPERTY(EditAnywhere, Category = "City|World")
    float CityRadius = 20000.f;

    UPROPERTY(EditAnywhere, Category = "City|World")
    int32 TargetHubCount = 6;

    UPROPERTY(EditAnywhere, Category = "City|World")
    int32 RandomSeed = 12345;

    
    
    

    UPROPERTY(EditAnywhere, Category = "City|Buildings")
    float BuildingDensity = 0.65f;

    UPROPERTY(EditAnywhere, Category = "City|Buildings")
    int32 TargetBuildingCount = 12000;

    
    
    

    UPROPERTY(EditAnywhere, Category = "City|Actors")
    TSubclassOf<ARoadActor> RoadClass;

    UPROPERTY(EditAnywhere, Category = "City|Actors")
    TSubclassOf<ABuildingActor> BuildingClass;
    UPROPERTY()
    TArray<FVector> RoadCenters;

    UPROPERTY()
    TArray<ARoadActor*> SpawnedRoads;
    void ClearRoads();

    UPROPERTY()
    TArray<FSidewalkSegment> SidewalkSegments;

    UPROPERTY(EditAnywhere, Category = "City|Sidewalk")
    float SidewalkWidth = 200.f;

    UPROPERTY(EditAnywhere, Category = "City|Sidewalk")
    float SidewalkOffset = 900.f;

    UPROPERTY(EditAnywhere, Category = "City|Sidewalk")
    UStaticMesh* SidewalkMesh;
private:

    
    
    

    void GenerateBlocksFromGrid();
    void GenerateGridRoads();

    TArray<FCityParcel> SubdivideBlockIntoParcels(const FCityBlock& Block);
    TArray<FCityCluster> ClusterParcels(const TArray<FCityParcel>& Parcels);
    void SpawnBuildingsFromClusters(const TArray<FCityCluster>& Clusters);

    void SpawnBuildingsFromParcels(const TArray<FCityParcel>& Parcels);

    
    
    

    void GenerateCity();
    void InitializeGenerator();
    void GenerateHubs();

    
    
    

    FRandomStream SimRandomStream;

    UPROPERTY()
    TArray<FCityHub> CityHubs;

    UPROPERTY()
    TArray<FCityBlock> CityBlocks;

    
    
    

    UPROPERTY()
    TArray<ARoadActor*> GeneratedRoads;

    UPROPERTY()
    TArray<ABuildingActor*> GeneratedBuildings;

    
    
    

    ARoadActor* SpawnRoadActor();
};