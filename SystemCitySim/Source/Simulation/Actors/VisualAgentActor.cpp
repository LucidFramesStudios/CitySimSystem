#include "Simulation/Actors/VisualAgentActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "Simulation/Actors/CityGeneratorActor.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Data/SimulationData.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Debug/DebugHUD.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "DrawDebugHelpers.h"

AVisualAgentActor::AVisualAgentActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 🔥 Throttle visual updates (huge perf win)
    PrimaryActorTick.TickInterval = 0.033f; // 30 FPS visual

    // 🔴 DISABLE CharacterMovement COMPLETELY
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->Deactivate();
        GetCharacterMovement()->DisableMovement();
        GetCharacterMovement()->SetComponentTickEnabled(false);
    }

    // 🔴 DISABLE CAPSULE COLLISION
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetCapsuleComponent()->SetGenerateOverlapEvents(false);
    }

    // 🔴 DISABLE MESH COLLISION
    if (GetMesh())
    {
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->SetGenerateOverlapEvents(false);

        // 🔥 Animation optimization (massive for 1000 agents)
        GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        GetMesh()->bEnableUpdateRateOptimizations = true;
    }

    // 🔴 REMOVE AI CONTROLLER (you don’t use it)
    AIControllerClass = nullptr;
    AutoPossessAI = EAutoPossessAI::Disabled;

    UE_LOG(LogSimulation, Log, TEXT("VisualAgentActor optimized (no movement/collision/controller)"));
}

void AVisualAgentActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SyncToSimulation(DeltaTime);

#if !(UE_BUILD_SHIPPING)
    if (ADebugHUD::bDebugAgents || ADebugHUD::bDebugPaths)
    {
        DrawAgentDebug();
    }
#endif
}



// System Boundary: Visual/Simulation Decoupling.
// NOTE: Relying on per-actor Tick and VInterpTo for visual synchronization becomes a CPU bottleneck at O(1k+) agents.

void AVisualAgentActor::SyncToSimulation(float DeltaTime)
{
    UNeedsSubsystem* NeedsSub = GetWorld()->GetSubsystem<UNeedsSubsystem>();
    if (!NeedsSub || AssignedAgentID < 0) return;

    FAgentData* Agent = NeedsSub->GetAgentData(AssignedAgentID);
    if (!Agent) return;

    
    SetActorHiddenInGame(false);

    FVector TargetLoc = Agent->LogicalLocation;
    SimVelocity = Agent->SimVelocity;

    if (Agent->bInsideBuilding)
    {
        // Seated at its reserved slot: render at the logical (slot) position and stop
        // moving, instead of scattering the agent up to 4.5m around the building.
        SimVelocity = FVector::ZeroVector;
    }

    TargetLoc.Z = 0.0f;
    FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetLoc, DeltaTime, 20.0f);
    SetActorLocation(NewLoc);

    float SpeedSq = SimVelocity.SizeSquared();

    if (SpeedSq > 10.f)
    {
        FRotator TargetRot = SimVelocity.Rotation();
        TargetRot.Pitch = 0.f; TargetRot.Roll = 0.f;
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 12.0f));
    }

    
    
    
    if (!Agent->bInsideBuilding)
    {
        if (SpeedSq > 90000.f) 
        {
            if (Agent->CurrentAnimationState != TEXT("Run"))
            {
                Agent->CurrentAnimationState = TEXT("Run");
            }
        }
        else if (SpeedSq > 15.f) 
        {
            
            if (Agent->CurrentAnimationState != TEXT("Run") || SpeedSq < 75000.f)
            {
                if (Agent->CurrentAnimationState != TEXT("Walk"))
                {
                    Agent->CurrentAnimationState = TEXT("Walk");
                }
            }
        }
        else if (SpeedSq < 5.f) 
        {
            if (Agent->CurrentAnimationState != TEXT("Idle"))
            {
                Agent->CurrentAnimationState = TEXT("Idle");
            }
        }
    }
}

void AVisualAgentActor::DrawAgentDebug()
{
    if (!ADebugHUD::bDebugAgents && !ADebugHUD::bDebugPaths)
        return;

    UWorld* World = GetWorld();
    if (!World) return;

    UNeedsSubsystem* Needs = World->GetSubsystem<UNeedsSubsystem>();
    if (!Needs) return;

    FAgentData* Agent = Needs->GetAgentData(AssignedAgentID);
    if (!Agent) return;

    
    if (ADebugHUD::bDebugPaths)
    {
        DrawDebugLine(
            World,
            GetActorLocation(),
            Agent->TargetLocation,
            FColor::Cyan,
            false,
            -1.f,
            0,
            2.f
        );

        for (const FVector& P : Agent->CurrentPath)
        {
            DrawDebugSphere(World, P, 25.f, 8, FColor::Green, false, -1.f);
        }
    }

    
    FString Reason = Agent->DecisionReason;
    if (Reason.Contains(TEXT("Rain")))
    {
        Reason = TEXT("");
    }

    
    FString LocTruth = Agent->bInsideBuilding
        ? FString::Printf(TEXT("Inside Obj: %d"), Agent->ActiveInteractableID)
        : TEXT("Outside");

    
    FString StateStr = UEnum::GetValueAsString(Agent->State);

    
   
}