#include "Simulation/Subsystems/CityEventSubsystem.h"
#include "Simulation/Subsystems/SimulationRandomSubsystem.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"

void UCityEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActiveEvent = ECityEvent::None;
}

void UCityEventSubsystem::ProcessTick(float WorldTime)
{
    
    if (ActiveEvent != ECityEvent::None && WorldTime >= EventEndTime)
    {
        EndCurrentEvent();
    }

    
    if (ActiveEvent == ECityEvent::None)
    {
        if ((WorldTime >= 8.0f && WorldTime < 9.0f) || (WorldTime >= 17.0f && WorldTime < 18.0f))
        {
            ActiveEvent = ECityEvent::RushHour;
            EventEndTime = WorldTime + 1.0f; 
            OnCityEventChanged.Broadcast(ActiveEvent);
            SIM_LOG_WARNING(-1, TEXT("CityEvent: Rush Hour Started"));
        }
        else
        {
            
            USimulationRandomSubsystem* Rand = GetWorld()->GetSubsystem<USimulationRandomSubsystem>();
            if (Rand && Rand->Rand() < 0.0005f) 
            {
                TriggerRandomEvent(WorldTime);
            }
        }
    }
}

void UCityEventSubsystem::TriggerRandomEvent(float CurrentWorldTime)
{
    USimulationRandomSubsystem* Rand = GetWorld()->GetSubsystem<USimulationRandomSubsystem>();
    if (!Rand) return;

    
    int32 Roll = Rand->RandInt(2, 4);
    ActiveEvent = static_cast<ECityEvent>(Roll);

    
    float Duration = Rand->RandRange(0.16f, 1.0f);
    EventEndTime = CurrentWorldTime + Duration;

    OnCityEventChanged.Broadcast(ActiveEvent);
    SIM_LOG_WARNING(-1, TEXT("CityEvent: %d Started. Ends at %.2f"), (int32)ActiveEvent, EventEndTime);
}

void UCityEventSubsystem::EndCurrentEvent()
{
    ActiveEvent = ECityEvent::None;
    OnCityEventChanged.Broadcast(ActiveEvent);
    SIM_LOG_WARNING(-1, TEXT("CityEvent: Ended"));
}

void UCityEventSubsystem::ForceEvent(ECityEvent NewEvent, float DurationInHours)
{
    ActiveEvent = NewEvent;

    if (UWorldSimulationSubsystem* Sim =
        GetWorld()->GetSubsystem<UWorldSimulationSubsystem>())
    {
        EventEndTime = Sim->WorldTime + DurationInHours;
    }

    OnCityEventChanged.Broadcast(ActiveEvent);

    UE_LOG(LogSimulation, Warning,
        TEXT("Forced City Event: %d"), (int32)ActiveEvent);
}
