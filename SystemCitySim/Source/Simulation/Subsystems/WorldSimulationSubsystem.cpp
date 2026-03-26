    #include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Subsystems/SimAISubsystem.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Data/SimulationData.h"
#include "Simulation/SimulationTags.h"
#include "Simulation/Subsystems/LifeSchedulerSubsystem.h"
#include "Simulation/Subsystems/PathfindingSubsystem.h"
#include "Simulation/Subsystems/TransitSubsystem.h"
#include "EngineUtils.h"
#include "Simulation/Subsystems/CityEventSubsystem.h"
#include "Simulation/Subsystems/SocialBehaviorSubsystem.h"
#include "Simulation/Subsystems/VehicleSimulationSubsystem.h"
#include "Simulation/Subsystems/RoadPathfindingSubsystem.h"
#include "HAL/PlatformTime.h"

void UWorldSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FSimTags::Init();   
    UE_LOG(LogSimulation, Log, TEXT("WorldSimulationSubsystem initialized"));
}

void UWorldSimulationSubsystem::Deinitialize()
{
    UE_LOG(LogSimulation, Log, TEXT("WorldSimulationSubsystem deinitialized"));

    Super::Deinitialize();
}


void UWorldSimulationSubsystem::Tick(float DeltaTime)
{
    
    Accumulator += DeltaTime;
    const int32 MaxStepsPerFrame = 3;
    int32 StepCount = 0;

    UPathfindingSubsystem* PathSub = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
    if (PathSub)
    {
        PathSub->ProcessResolvedPaths();
    }

    
    while (Accumulator >= FixedTickRate && StepCount < MaxStepsPerFrame)
    {
        double StartTime = FPlatformTime::Seconds();
        ExecuteSimulationStep();
        double EndTime = FPlatformTime::Seconds();

        
        LastSimulationStepMS = (EndTime - StartTime) * 1000.0;
        AverageSimulationStepMS = (AverageSimulationStepMS * 0.9f) + (LastSimulationStepMS * 0.1f);
        Accumulator -= FixedTickRate;
        StepCount++;
    }
}





void UWorldSimulationSubsystem::ExecuteSimulationStep()
{
    
    float ScaledTickRate = FixedTickRate * TimeScale;
    float HoursPerFixedTick = 24.0f / DayDurationSeconds;

    
    WorldTime = FMath::Fmod(WorldTime + (HoursPerFixedTick * ScaledTickRate), 24.0f);
    OnTimeChanged.Broadcast(WorldTime);

    UWorld* World = GetWorld();
    if (!World) return;

    UNeedsSubsystem* NeedsSub = World->GetSubsystem<UNeedsSubsystem>();
    ULifeSchedulerSubsystem* Scheduler = World->GetSubsystem<ULifeSchedulerSubsystem>();
    USimAISubsystem* SimAI = World->GetSubsystem<USimAISubsystem>();
    UInteractionSubsystem* IntSub = World->GetSubsystem<UInteractionSubsystem>();
    UTransitSubsystem* TransitSub = World->GetSubsystem<UTransitSubsystem>();
    UVehicleSimulationSubsystem* VehSub = World->GetSubsystem<UVehicleSimulationSubsystem>();

    if (!NeedsSub || !Scheduler || !SimAI || !IntSub) return;

    TArray<FAgentData>& Agents = NeedsSub->GetActiveAgents();

    for (FAgentData& Agent : Agents)
    {
        
        Scheduler->EvaluateAgentSchedule(Agent, WorldTime);
        SimAI->EvaluateCommitment(Agent, ScaledTickRate);

        if (!Agent.bIsCommittedToGoal && !Agent.bInsideBuilding)
        {
            NeedsSub->ScoreAgentNeeds(Agent, ScaledTickRate);
            SimAI->MakeDecision(Agent, WorldTime);
        }

        SimAI->ExecuteMovement(Agent, ScaledTickRate, Agents);
        IntSub->ProcessAgentInteraction(Agent, ScaledTickRate);
    }

    if (TransitSub) TransitSub->ProcessTick(ScaledTickRate);
    if (VehSub) VehSub->ProcessTick(ScaledTickRate);

    
    
    if (IntSub) IntSub->ProcessTick(ScaledTickRate);
}
 

void UWorldSimulationSubsystem::DumpSimulationState()
{
    UNeedsSubsystem* NeedsSub = GetWorld()->GetSubsystem<UNeedsSubsystem>();
    if (!NeedsSub) return;

    for (const FAgentData& Agent : NeedsSub->GetActiveAgents())
    {
        
        SIM_LOG_INFO(Agent.AgentID, TEXT("Action: %s | Goal: %s | Target: %d | Timer: %.2f"),
            *Agent.ActiveGoal.ToString(), *Agent.ActiveGoal.ToString(), Agent.ActiveInteractableID, Agent.InteractionRemainingTime);

        for (const auto& Need : Agent.Needs)
        {
            SIM_LOG_INFO(Agent.AgentID, TEXT("  Need [%s]: %.2f"), *Need.Key.ToString(), Need.Value);
        }
    }
}

void UWorldSimulationSubsystem::SetRain(bool bRainActive)
{
    bIsRaining = bRainActive;
}