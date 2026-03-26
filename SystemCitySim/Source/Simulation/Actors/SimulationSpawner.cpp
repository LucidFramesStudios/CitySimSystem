#include "Simulation/Actors/SimulationSpawner.h"
#include "Simulation/Actors/VisualAgentActor.h"
#include "Components/SplineComponent.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "Simulation/Subsystems/LifeAssignmentSubsystem.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Subsystems/VehicleSimulationSubsystem.h"
#include "Simulation/Actors/VisualVehicleActor.h"

void ASimulationSpawner::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    World->GetTimerManager().SetTimer(
        SpawnTimerHandle,
        this,
        &ASimulationSpawner::SpawnAgentsSafe,
        0.5f, 
        false
    );
}

void ASimulationSpawner::SpawnVehicles(UVehicleSimulationSubsystem* VehSub)
{
    if (!VehSub || !VisualVehicleClass)
        return;

    for (const FVehicleData& Veh : VehSub->GetActiveVehicles())
    {
        FVector SpawnLoc = Veh.LogicalLocation;
        FRotator SpawnRot = Veh.LogicalRotation;

        
        

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AVisualVehicleActor* Actor =
            GetWorld()->SpawnActor<AVisualVehicleActor>(
                VisualVehicleClass,
                SpawnLoc,
                SpawnRot,
                Params);

        if (Actor)
        {
            Actor->AssignedVehicleID = Veh.VehicleID;

            UE_LOG(LogSimulation, Log,
                TEXT("Spawned Vehicle %d at %s"),
                Veh.VehicleID,
                *SpawnLoc.ToString());
        }
    }
}

void ASimulationSpawner::SpawnAgentsSafe()
{
    UWorld* InnerWorld = GetWorld();
    if (!IsValid(InnerWorld)) return;

    UNeedsSubsystem* Needs =
        InnerWorld->GetSubsystem<UNeedsSubsystem>();

    ULifeAssignmentSubsystem* LifeSub =
        InnerWorld->GetSubsystem<ULifeAssignmentSubsystem>();

    if (!IsValid(Needs) || !IsValid(LifeSub))
    {
        UE_LOG(LogSimulation, Error,
            TEXT("Spawner aborted: required subsystems missing"));
        return;
    }

    
    if (Needs->GetActiveAgents().Num() == 0)
    {
        UE_LOG(LogSimulation, Error,
            TEXT("No agents available yet"));
        return;
    }

    LifeSub->ExecuteLifeAssignments();

    UInteractionSubsystem* IntSub = InnerWorld->GetSubsystem<UInteractionSubsystem>();

    AgentCount = Needs->GetActiveAgents().Num();

    for (int32 i = 0; i < AgentCount; i++)
    {
        FAgentData* Agent = Needs->GetAgentData(i);
        if (!Agent) continue;

        if (Agent->HomeObjectID == -1)
        {
            UE_LOG(LogSimulation, Error, TEXT("Spawn failed: Agent %d has no home"), i);
            continue;
        }

        const USmartObjectComponent* HomeObj =
            IntSub->GetSmartObject(Agent->HomeObjectID);

        if (!HomeObj)
        {
            UE_LOG(LogSimulation, Error, TEXT("Invalid Home Object for Agent %d"), i);
            continue;
        }

        FVector HomeLoc = HomeObj->EntryLocation;

        Agent->LogicalLocation = HomeLoc;
        Agent->TargetLocation = HomeLoc;

        
        Agent->CurrentPath.Empty();
        Agent->DesiredTargetID = -1;
        Agent->ActiveInteractableID = -1;

        Agent->State = EAgentState::Idle;
        Agent->StateTimer = 2.0f;

        Agent->bInsideBuilding = false;

        
        Agent->DesiredGoal = FGameplayTag();
        Agent->ActiveGoal = FGameplayTag();

        
        Agent->CurrentPhase = ELifePhase::Sleep;
    }

    AgentCount = Needs->GetActiveAgents().Num();

    for (int32 i = 0; i < AgentCount; i++)
    {
        FAgentData* Agent = Needs->GetAgentData(i);
        if (!Agent) continue;

        FVector SpawnLoc = Agent->LogicalLocation;
        SpawnLoc.Z  = 7.f;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AVisualAgentActor* Actor =
            InnerWorld->SpawnActor<AVisualAgentActor>(
                VisualAgentClass,
                SpawnLoc,
                FRotator::ZeroRotator,
                Params);

        if (IsValid(Actor))
        {
            Actor->AssignedAgentID = i;
        }
    }

    
    if (UVehicleSimulationSubsystem* Veh =
        InnerWorld->GetSubsystem<UVehicleSimulationSubsystem>())
    {
        SpawnVehicles(Veh);
    }

    UE_LOG(LogSimulation, Log, TEXT("Agents spawned safely"));
}