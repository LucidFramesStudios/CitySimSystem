#include "Simulation/Debug/DebugHUD.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Subsystems/VehicleSimulationSubsystem.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "Simulation/Subsystems/RoadGraphSubsystem.h"

#include "Engine/Canvas.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"




bool ADebugHUD::bDebugAgents = false;
bool ADebugHUD::bDebugPaths = false;
bool ADebugHUD::bDebugStats = false;
bool ADebugHUD::bDebugVehicles = false;

void ADebugHUD::BeginPlay()
{
    Super::BeginPlay();

    GetWorld()->GetTimerManager().SetTimer(
        MetricsTimerHandle,
        this,
        &ADebugHUD::UpdatePerformanceMetrics,
        1.0f,
        true);
}
FORCEINLINE FColor GetAgentDebugColor(int32 AgentID)
{
    
    uint32 Hash = GetTypeHash(AgentID);

    uint8 R = (Hash & 0xFF);
    uint8 G = (Hash >> 8) & 0xFF;
    uint8 B = (Hash >> 16) & 0xFF;

    
    const uint8 MinBrightness = 100;
    R = FMath::Max(R, MinBrightness);
    G = FMath::Max(G, MinBrightness);
    B = FMath::Max(B, MinBrightness);

    return FColor(R, G, B, 255);
}




void ADebugHUD::UpdatePerformanceMetrics()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UNeedsSubsystem* Needs =
        World->GetSubsystem<UNeedsSubsystem>();

    UVehicleSimulationSubsystem* VehSub =
        World->GetSubsystem<UVehicleSimulationSubsystem>();

    UInteractionSubsystem* IntSub =
        World->GetSubsystem<UInteractionSubsystem>();

    UWorldSimulationSubsystem* WorldSim =
        World->GetSubsystem<UWorldSimulationSubsystem>();

    int32 AgentCount = Needs ?
        Needs->GetActiveAgents().Num() : 0;

    int32 VehicleCount = VehSub ?
        VehSub->GetActiveVehicles().Num() : 0;

    
    
    

    int32 ActiveInteractions = 0;

    if (IntSub)
    {
        for (const FSmartObjectEntry& Entry :
            IntSub->GetAvailableObjects())
        {
            if (Entry.Component)
            {
                ActiveInteractions +=
                    Entry.Component->CurrentUsers;
            }
        }
    }

    
    
    

    float WorldTime = 0.f;
    float StepMS = 0.f;
    float AvgMS = 0.f;

    if (WorldSim)
    {
        WorldTime = WorldSim->WorldTime;

        StepMS =
            WorldSim->GetLastSimulationStepMS();

        AvgMS =
            WorldSim->GetAverageSimulationStepMS();
    }


    
    
    

    CachedMetricsText =
        FString::Printf(
            TEXT("=== SIMULATION METRICS ===\n"
                "Agents: %d\n"
                "Vehicles: %d\n"
                "Interactions: %d\n"
                "World Time: %.2f\n"
                "Sim Step: %.3f ms\n"
                "Sim Avg: %.3f ms"),
            AgentCount,
            VehicleCount,
            ActiveInteractions,
            WorldTime,
            StepMS,
            AvgMS);

}





void ADebugHUD::DrawPerformanceMetrics()
{
    /*if (CachedMetricsText.IsEmpty())
        return;
     
    if (!bDebugStats)
    {
        return;
    }
     
    DrawText(
        CachedMetricsText,
        FColor::Green,
        20.f,
        20.f,
        GEngine->GetMediumFont());*/
}










void ADebugHUD::DrawHUD()
{
    Super::DrawHUD();

    UWorld* World = GetWorld();
    if (!World) return;

    if (UWorldSimulationSubsystem* Sim = World->GetSubsystem<UWorldSimulationSubsystem>())
    {
        if (Sim->bIsRaining)
        {
            DrawRain(World->GetDeltaSeconds());
        }
    }

    if (bShowDensityDebug)
    {
        DrawCrowdDensity();
        DrawFlowLines();
    }

    DrawSimStats();

    
    
    if (bDebugAgents)
    {
        UNeedsSubsystem* Needs = World->GetSubsystem<UNeedsSubsystem>();
        APlayerController* PC = World->GetFirstPlayerController();

        if (Needs && PC && PC->GetPawn())
        {
            FVector CameraLoc = PC->GetPawn()->GetActorLocation();

            
            TArray<FAgentData> SortedAgents;
            SortedAgents.Append(Needs->GetActiveAgents());

            
            SortedAgents.Sort([CameraLoc](const FAgentData& A, const FAgentData& B)
                {
                    return FVector::DistSquared(CameraLoc, A.LogicalLocation)
                        < FVector::DistSquared(CameraLoc, B.LogicalLocation);
                });

            const int32 MaxDrawAgents = 50;
            int32 DrawCount = FMath::Min(MaxDrawAgents, SortedAgents.Num());

            for (int32 i = 0; i < DrawCount; i++)
            {
                const FAgentData& Agent = SortedAgents[i];

                FVector ScreenPos = Project(Agent.LogicalLocation);
                if (ScreenPos.Z <= 0.f)
                    continue;

                
                if (i == 0)
                {
                    const float Radius = 50.f;
                    const int32 Segments = 24;

                    for (int32 j = 0; j < Segments; j++)
                    {
                        float A1 = (j / (float)Segments) * 2 * PI;
                        float A2 = ((j + 1) / (float)Segments) * 2 * PI;

                        float X1 = ScreenPos.X + FMath::Cos(A1) * Radius;
                        float Y1 = ScreenPos.Y + FMath::Sin(A1) * Radius;

                        float X2 = ScreenPos.X + FMath::Cos(A2) * Radius;
                        float Y2 = ScreenPos.Y + FMath::Sin(A2) * Radius;

                        DrawLine(X1, Y1, X2, Y2, FLinearColor::White, 2.f);
                    }
                }

                FString StateStr;
                switch (Agent.State)
                {
                case EAgentState::Decision: StateStr = TEXT("Decision"); break;
                case EAgentState::Traveling: StateStr = TEXT("Traveling"); break;
                case EAgentState::FinalApproach: StateStr = TEXT("FinalApproach"); break;
                case EAgentState::Interacting: StateStr = TEXT("Interacting"); break;
                case EAgentState::WaitingForPath: StateStr = TEXT("WaitingForPath"); break;
                case EAgentState::Reaction: StateStr = TEXT("Reaction"); break;
                case EAgentState::Idle: StateStr = TEXT("Idle"); break;
                case EAgentState::Cooldown: StateStr = TEXT("Cooldown"); break;
                default: StateStr = TEXT("Unknown"); break;
                }

                FString LocTruth = Agent.bInsideBuilding
                    ? FString::Printf(TEXT("Inside Obj: %d"), Agent.ActiveInteractableID)
                    : TEXT("Outside");

                FString Reason = Agent.DecisionReason;
                if (Reason.Contains(TEXT("Rain")))
                {
                    Reason = TEXT("");
                }

                FString Txt = FString::Printf(
                    TEXT("Agent %d\n\nState: %s\nPhase: %d\n\nGoal: %s\n\nLoc: %s\n\nAnim: %s\nSpd: %.1f\n\n%s"),
                    Agent.AgentID,
                    *StateStr,
                    (int32)Agent.CurrentPhase,
                    *Agent.DesiredGoal.ToString(),
                    *LocTruth,
                    *Agent.CurrentAnimationState,
                    Agent.SimVelocity.Size(),
                    *Reason
                );

                
                
                FColor AgentColor = GetAgentDebugColor(Agent.AgentID);

                DrawText(
                    Txt,
                    AgentColor,
                    ScreenPos.X,
                    ScreenPos.Y - 80.f,
                    GEngine->GetSmallFont()
                );
            }
        }
    }

    
    if (bShowSocialDebug) DrawSocialLinks();
    if (bShowTransitDebug) DrawBusRoutes();

    
    if (bShowZoneDebug)
    {
        DrawText(TEXT("WARNING: Zone Debug active, but DrawZones() is not implemented."), FColor::Red, 20.f, 250.f, GEngine->GetMediumFont());
    }

    
    if (bDebugVehicles)
    {
        if (UVehicleSimulationSubsystem* VehSub = World->GetSubsystem<UVehicleSimulationSubsystem>())
        {
            int32 DrawnVehicles = 0;

            for (const FVehicleData& Veh : VehSub->GetActiveVehicles())
            {
                if (DrawnVehicles > 50) break;

                FVector ScreenPos = Project(Veh.LogicalLocation);
                if (ScreenPos.Z > 0.f)
                {
                    DrawText(
                        FString::Printf(TEXT("Veh %d"), Veh.VehicleID),
                        FColor::Orange,
                        ScreenPos.X,
                        ScreenPos.Y,
                        GEngine->GetSmallFont()
                    );
                }

                DrawnVehicles++;
            }
        }
    }
}





 

void ADebugHUD::Sim_DebugTransit()
{
    bShowTransitDebug = !bShowTransitDebug;
}

void ADebugHUD::Sim_DebugZones()
{
    bShowZoneDebug = !bShowZoneDebug;
}

void ADebugHUD::Sim_DebugDensity()
{
    bShowDensityDebug = !bShowDensityDebug;
}


void ADebugHUD::DrawCrowdDensity()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UInteractionSubsystem* IntSub =
        World->GetSubsystem<UInteractionSubsystem>();

    if (!IntSub) return;

    const float CellSize = 400.f;

    for (const auto& Pair : IntSub->SpatialGrid)
    {
        const FIntVector& Cell = Pair.Key;
        const FSpatialCell& Data = Pair.Value;

        if (Data.AgentCount == 0)
            continue;

        FVector Center(
            (Cell.X + 0.5f) * CellSize,
            (Cell.Y + 0.5f) * CellSize,
            500.f
        );

        float Density = FMath::Clamp(Data.AgentCount / 10.f, 0.f, 1.f);

        FColor Color = FColor::Blue;

        if (Density > 0.75f) Color = FColor::Red;
        else if (Density > 0.5f) Color = FColor::Yellow;
        else if (Density > 0.25f) Color = FColor::Green;

        
        DrawDebugBox(
            World,
            Center,
            FVector(CellSize * 0.5f, CellSize * 0.5f, 10.f),
            Color,
            false,
            -1.f,
            0,
            5.f
        );
    }
}


void ADebugHUD::DrawBusRoutes()
{
    UVehicleSimulationSubsystem* VehSub =
        GetWorld()->GetSubsystem<UVehicleSimulationSubsystem>();

    URoadGraphSubsystem* RoadGraph =
        GetWorld()->GetSubsystem<URoadGraphSubsystem>();

    if (!VehSub || !RoadGraph)
        return;

    for (const FVehicleData& Veh : VehSub->GetActiveVehicles())
    {
        for (int32 EdgeID : Veh.EdgeRoute)
        {
            const FRoadEdge* Edge = RoadGraph->GetEdge(EdgeID);

            if (!Edge)
                continue;

            DrawDebugLine(
                GetWorld(),
                Edge->StartLocation,
                Edge->EndLocation,
                FColor::Cyan,
                false,
                -1.f,
                0,
                4.f
            );
        }
    }
}


void ADebugHUD::Sim_DebugSocial()
{
    bShowSocialDebug = !bShowSocialDebug;
}
void ADebugHUD::DrawSocialLinks()
{
    UNeedsSubsystem* Needs =
        GetWorld()->GetSubsystem<UNeedsSubsystem>();

    if (!Needs)
        return;

    for (const FAgentData& Agent : Needs->GetActiveAgents())
    {
        for (int32 FriendID : Agent.Friends)
        {
            const FAgentData* Friend =
                Needs->GetAgentData(FriendID);

            if (!Friend)
                continue;

            DrawDebugLine(
                GetWorld(),
                Agent.LogicalLocation,
                Friend->LogicalLocation,
                FColor::Purple,
                false,
                -1.f,
                0,
                2.f
            );
        }
    }
}



void ADebugHUD::DrawRain(float DeltaTime)
{
    float Time = GetWorld()->GetTimeSeconds();
    float ScreenX = Canvas->SizeX;
    float ScreenY = Canvas->SizeY;

    
    for (int32 i = 0; i < 300; ++i)
    {
        
        float SeedX = (i * 37) % (int32)ScreenX;
        float SeedY = (i * 101) % (int32)ScreenY;

        
        float X = FMath::Fmod(SeedX + (Time * 400.f) + (i * 2.f), ScreenX);
        float Y = FMath::Fmod(SeedY + (Time * 1800.f) + (i * 5.f), ScreenY);

        float Length = 15.f + (i % 25);

        
        FLinearColor RainColor(0.7f, 0.8f, 0.9f, 0.3f + ((i % 10) * 0.05f));

        DrawLine(X, Y, X + (Length * 0.15f), Y + Length, RainColor, 1.0f);
    }
}

void ADebugHUD::Sim_DebugAgents() { bDebugAgents = !bDebugAgents; }
void ADebugHUD::Sim_DebugPaths() { bDebugPaths = !bDebugPaths; }
void ADebugHUD::Sim_DebugStats() { bDebugStats = !bDebugStats; }
void ADebugHUD::Sim_DebugVehicles() { bDebugVehicles = !bDebugVehicles; }


void ADebugHUD::DrawSimStats()
{
    if (!bDebugStats) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UNeedsSubsystem* Needs = World->GetSubsystem<UNeedsSubsystem>();
    UVehicleSimulationSubsystem* VehSub = World->GetSubsystem<UVehicleSimulationSubsystem>();
    UWorldSimulationSubsystem* Sim = World->GetSubsystem<UWorldSimulationSubsystem>();

    if (!Needs || !Sim || !VehSub) return;

    int32 AgentCount = Needs->GetActiveAgents().Num();
    int32 VehicleCount = VehSub->GetActiveVehicles().Num();

    int32 Sleep = 0, Work = 0, Leisure = 0;
    for (const FAgentData& A : Needs->GetActiveAgents())
    {
        if (A.CurrentPhase == ELifePhase::Sleep) Sleep++;
        else if (A.CurrentPhase == ELifePhase::Work) Work++;
        else Leisure++;
    }

    float StepMS = Sim->GetLastSimulationStepMS();
    float AvgMS = Sim->GetAverageSimulationStepMS();

    FString Txt = FString::Printf(
        TEXT("=== SIMULATION ===\nAgents: %d\nVehicles: %d\nTime: %.2f\nRain: %s\nSleep: %d\nWork: %d\nLeisure: %d\nStep: %.3f ms\nAvg: %.3f ms"),
        AgentCount,
        VehicleCount,
        Sim->WorldTime,
        Sim->bIsRaining ? TEXT("ON") : TEXT("OFF"),
        Sleep,
        Work,
        Leisure,
        StepMS,
        AvgMS
    );

    DrawText(Txt, FColor::Cyan, 20.f, 20.f, GEngine->GetMediumFont());
}

void ADebugHUD::Sim_SetTime(float NewTime)
{
    if (UWorldSimulationSubsystem* Sim = GetWorld()->GetSubsystem<UWorldSimulationSubsystem>())
    {
        Sim->WorldTime = FMath::Fmod(NewTime, 24.0f);
        Sim->OnTimeChanged.Broadcast(Sim->WorldTime);
    }
}

void ADebugHUD::DrawFlowLines()
{
    UNeedsSubsystem* Needs = GetWorld()->GetSubsystem<UNeedsSubsystem>();
    if (!Needs) return;

    int32 Count = 0;
    const int32 MaxLines = 80;

    for (const FAgentData& Agent : Needs->GetActiveAgents())
    {
        if (Count > MaxLines) break;

        if (Agent.SimVelocity.SizeSquared() < 10.f)
            continue;

        FVector Start = Agent.LogicalLocation;
        FVector End = Start + Agent.SimVelocity.GetSafeNormal() * 200.f;

        
        DrawDebugLine(
            GetWorld(),
            Start,
            End,
            FColor::Cyan,
            false,
            -1.f,
            0,
            12.f
        );

        Count++;
    }
}

void ADebugHUD::DrawFocusCircle(const FVector& WorldLoc)
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return;

    FVector2D ScreenPos;
    bool bProjected = PC->ProjectWorldLocationToScreen(WorldLoc, ScreenPos);

    if (!bProjected)
        return;

    const float Radius = 60.f;
    const int32 Segments = 32;

    for (int32 i = 0; i < Segments; i++)
    {
        float Angle1 = (i / (float)Segments) * 2 * PI;
        float Angle2 = ((i + 1) / (float)Segments) * 2 * PI;

        float X1 = ScreenPos.X + FMath::Cos(Angle1) * Radius;
        float Y1 = ScreenPos.Y + FMath::Sin(Angle1) * Radius;

        float X2 = ScreenPos.X + FMath::Cos(Angle2) * Radius;
        float Y2 = ScreenPos.Y + FMath::Sin(Angle2) * Radius;

        DrawLine(X1, Y1, X2, Y2, FLinearColor::White, 2.f);
    }
}