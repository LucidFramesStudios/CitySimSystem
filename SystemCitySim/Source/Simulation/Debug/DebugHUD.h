#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DebugHUD.generated.h"

UCLASS()
class ADebugHUD : public AHUD
{
    GENERATED_BODY()

public:

    virtual void BeginPlay() override;
    virtual void DrawHUD() override;

    
    void  DrawFlowLines();
    void DrawFocusCircle(const FVector& WorldLoc);
    UFUNCTION(Exec)
    void Sim_DebugTransit();

    UFUNCTION(Exec)
    void Sim_DebugZones();

    UFUNCTION(Exec)
    void Sim_DebugDensity();

    bool bShowTransitDebug = false;
    bool bShowZoneDebug = false;
    bool bShowDensityDebug = false;

    bool bShowSocialDebug = false;

    UFUNCTION(Exec)
    void Sim_DebugSocial();
    UFUNCTION(Exec) void Sim_DebugAgents();
    UFUNCTION(Exec) void Sim_DebugPaths();
    UFUNCTION(Exec) void Sim_DebugStats();
    UFUNCTION(Exec) void Sim_DebugVehicles();

    static bool bDebugAgents;
    static bool bDebugPaths;
    static bool bDebugStats;
    static bool bDebugVehicles;
    void DrawSimStats();

    UFUNCTION(Exec)
    void Sim_SetTime(float NewTime);

private:
    void DrawRain(float DeltaTime);
     

    
    
    
    void DrawCrowdDensity();

    FTimerHandle MetricsTimerHandle;

    FString CachedMetricsText;

    void UpdatePerformanceMetrics();
    void DrawBusRoutes();
    void DrawSocialLinks();
    void DrawPerformanceMetrics();
};
