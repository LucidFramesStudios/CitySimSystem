#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "VehicleSimulationSubsystem.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API UVehicleSimulationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;  
    void ProcessTick(float FixedTickRate);  
    bool DispatchTaxi(int32 AgentID, FVector AgentLoc, int32 StartNode, int32 TargetNode);
    TArray<FVehicleData>& GetActiveVehicles() { return ActiveVehicles; }  

    void BuildRoadNetwork();  
    void InitializeRoads();  
    const FVehicleData* GetVehicleData(int32 VehicleID) const;
    UFUNCTION(Exec)
    void Sim_DebugTransit();

private:
    TArray<FVehicleData> ActiveVehicles;  

    // 1D Spatial Partitioning: EdgeID -> Array of Vehicle Indices
    TMap<int32, TArray<int32>> EdgeVehicleMap;

    bool GenerateRoute(FVehicleData& Vehicle, int32 StartNodeID, int32 TargetNodeID);
    void HandleVehicleProgression(FVehicleData& Vehicle, const struct FRoadEdge* CurrentEdge);

    bool bDebugTransit = false;

    UPROPERTY()
    TMap<int32, int32> VehicleIDToIndex;
};