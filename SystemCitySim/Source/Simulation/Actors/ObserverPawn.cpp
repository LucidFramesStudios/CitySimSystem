#include "Simulation/Actors/ObserverPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "GameFramework/SpringArmComponent.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "EngineUtils.h" 
#include "Simulation/Actors/VisualAgentActor.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Debug/DebugHUD.h"
 

#include "Engine/World.h"

AObserverPawn::AObserverPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    MovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    MovementComp->MaxSpeed = 4000.f; 

    
    SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    RootComponent = SpringArmComp;
    SpringArmComp->bDoCollisionTest = false;
    SpringArmComp->TargetArmLength = 2500.f;
    SpringArmComp->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
    SpringArmComp->bEnableCameraLag = true;
    SpringArmComp->CameraLagSpeed = 10.0f;

    SpringArmComp->bEnableCameraRotationLag = true;
    SpringArmComp->CameraRotationLagSpeed = 10.0f;
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName); 
}

void AObserverPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World) return;

    const float UnscaledDelta = World->GetDeltaSeconds();

    switch (CurrentMode)
    {
    case EObserverMode::Follow:
    {
        UNeedsSubsystem* Needs = World->GetSubsystem<UNeedsSubsystem>();
        if (!Needs) break;

        const TArray<FAgentData>& Agents = Needs->GetActiveAgents();
        const int32 NumAgents = Agents.Num();
        if (NumAgents == 0) break;

        FollowedAgentIndex = FMath::Clamp(FollowedAgentIndex, 0, NumAgents - 1);
        const FAgentData& Agent = Agents[FollowedAgentIndex];

        //   SMOOTH TARGET (prevents jitter from sim stepping)
        static FVector SmoothedTarget = FVector::ZeroVector;

        SmoothedTarget = FMath::VInterpTo(
            SmoothedTarget,
            Agent.LogicalLocation,
            UnscaledDelta,
            10.0f
        );

        const FVector TargetLoc = SmoothedTarget;

        //   DIRECTIONAL FOLLOW (instead of fixed offset)
        FVector BackDir = -Agent.SimVelocity.GetSafeNormal();

        if (BackDir.IsNearlyZero())
        {
            BackDir = -GetActorForwardVector();
        }

        //   CLOSE CAMERA (tuned values)
        float FollowDistance = Agent.bInsideBuilding ? 1400.f : 800.f;
        float HeightOffset = Agent.bInsideBuilding ? 1150.f : 1150.f;

        if (UWorldSimulationSubsystem* Sim = World->GetSubsystem<UWorldSimulationSubsystem>())
        {
            if (Sim->bIsRaining)
            {
                FollowDistance *= 1.2f;
            }
        }

        FVector Offset = BackDir * FollowDistance + FVector(0, 0, HeightOffset);

        const FVector DesiredLocation = TargetLoc + Offset + FollowOffsetInput;

        //   NO INTERP HERE (SpringArm handles smoothing)
        SetActorLocation(DesiredLocation);

        //   LOOK AT AGENT
        const FRotator TargetRotation = (TargetLoc - DesiredLocation).Rotation();

        const FRotator NewRotation = FMath::RInterpTo(
            GetActorRotation(),
            TargetRotation,
            UnscaledDelta,
            8.0f
        );

        SetActorRotation(NewRotation);

        //   FORCE CLOSE CAMERA (override spring arm distance)
        SpringArmComp->TargetArmLength = FMath::FInterpTo(
            SpringArmComp->TargetArmLength,
            500.f,
            UnscaledDelta,
            6.0f
        );
        //   DAMPING (prevents infinite drift)
        FollowOffsetInput = FMath::VInterpTo(
            FollowOffsetInput,
            FVector::ZeroVector,
            UnscaledDelta,
            1.2f
        );

        //   OPTIONAL CLAMP (prevents crazy offsets)
        FollowOffsetInput = FollowOffsetInput.GetClampedToMaxSize(800.f);
        break;
    }

    case EObserverMode::Orbit:
    {
        const FVector Center = FVector::ZeroVector;
        const float Time = World->GetTimeSeconds();
        const float OrbitSpeed = 0.2f;
        const float Radius = 12000.f;
        const float Height = 3000.f;

        const float Angle = Time * OrbitSpeed;

        const FVector DesiredLocation = Center + FVector(
            FMath::Cos(Angle) * Radius,
            FMath::Sin(Angle) * Radius,
            Height
        );

        const FVector NewLocation = FMath::VInterpTo(
            GetActorLocation(),
            DesiredLocation,
            UnscaledDelta,
            1.5f
        );

        SetActorLocation(NewLocation);

        const FRotator LookAt = (Center - NewLocation).Rotation();

        const FRotator NewRotation = FMath::RInterpTo(
            GetActorRotation(),
            LookAt,
            UnscaledDelta,
            2.0f
        );

        SetActorRotation(NewRotation);

        break;
    }

    case EObserverMode::Free:
    {
        break;
    }

    default:
        break;
    }

    

    if (UWorldSimulationSubsystem* Sim = World->GetSubsystem<UWorldSimulationSubsystem>())
    {
        float TargetFOV = Sim->bIsRaining ? 85.f : 90.f;

        CameraComp->SetFieldOfView(
            FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, UnscaledDelta, 2.0f)
        );
    }
}


void AObserverPawn::MoveForward(float Val)
{
    if (Val == 0.f) return;

    if (CurrentMode == EObserverMode::Follow)
    {
        const FVector CamForward = CameraComp->GetForwardVector();
        FollowOffsetInput += CamForward * Val * 120.f;
        return;
    }

    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Val);
    }
}

void AObserverPawn::MoveRight(float Val)
{
    if (Val == 0.f) return;

    if (CurrentMode == EObserverMode::Follow)
    {
        const FVector CamRight = CameraComp->GetRightVector();
        FollowOffsetInput += CamRight * Val * 120.f;
        return;
    }

    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Val);
    }
}

void AObserverPawn::MoveUp(float Val)
{
    if (Val == 0.f) return;

    if (CurrentMode == EObserverMode::Follow)
    {
        FollowOffsetInput += FVector::UpVector * Val * 120.f;
        return;
    }

    AddMovementInput(GetActorUpVector(), Val);
}

void AObserverPawn::SetTimeScale1x()
{
    SetTimeScale(1.0f);
}

void AObserverPawn::SetTimeScale5x()
{
    SetTimeScale(5.0f);
}

void AObserverPawn::SetTimeScale20x()
{
    SetTimeScale(20.0f);
}

void AObserverPawn::SetTimeScale(float NewScale)
{
    if (UWorldSimulationSubsystem* Sim =
        GetWorld()->GetSubsystem<UWorldSimulationSubsystem>())
    {
        Sim->TimeScale = NewScale;
    }
}

void AObserverPawn::ToggleRushHour()
{
    TriggerEvent(ECityEvent::RushHour);
}

void AObserverPawn::ToggleRain()
{
    if (UWorldSimulationSubsystem* Sim = GetWorld()->GetSubsystem<UWorldSimulationSubsystem>())
    {
        Sim->SetRain(!Sim->bIsRaining);
    }
}

void AObserverPawn::ToggleFestival()
{
    TriggerEvent(ECityEvent::Festival);
}

void AObserverPawn::TriggerEvent(ECityEvent EventType)
{
    UCityEventSubsystem* EventSub =
        GetWorld()->GetSubsystem<UCityEventSubsystem>();

    if (EventSub)
    {
        EventSub->ForceEvent(EventType, 1.0f);
    }
}


void AObserverPawn::ToggleHUD()
{
    if (APlayerController* PC =
        Cast<APlayerController>(GetController()))
    {
        if (ADebugHUD* HUD =
            Cast<ADebugHUD>(PC->GetHUD()))
        {
            HUD->Sim_DebugAgents();
        }
    }
}

void AObserverPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AObserverPawn::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AObserverPawn::MoveRight);
    PlayerInputComponent->BindAxis("MoveUp", this, &AObserverPawn::MoveUp);

    
    
    

    
    PlayerInputComponent->BindAxis("Turn", this, &AObserverPawn::TurnCamera);
    PlayerInputComponent->BindAxis("LookUp", this, &AObserverPawn::LookUpCamera);

    PlayerInputComponent->BindAction("Time1x", IE_Pressed, this, &AObserverPawn::SetTimeScale1x);
    PlayerInputComponent->BindAction("Time5x", IE_Pressed, this, &AObserverPawn::SetTimeScale5x);
    PlayerInputComponent->BindAction("Time20x", IE_Pressed, this, &AObserverPawn::SetTimeScale20x);

    PlayerInputComponent->BindAction("EventRushHour", IE_Pressed, this, &AObserverPawn::ToggleRushHour);
    PlayerInputComponent->BindAction("EventRain", IE_Pressed, this, &AObserverPawn::ToggleRain);
    PlayerInputComponent->BindAction("EventFestival", IE_Pressed, this, &AObserverPawn::ToggleFestival);

    PlayerInputComponent->BindAction("ToggleHUD", IE_Pressed, this, &AObserverPawn::ToggleHUD);

    PlayerInputComponent->BindAction("CameraOrbit", IE_Pressed, this, &AObserverPawn::SetModeOrbit);
    PlayerInputComponent->BindAction("CameraFree", IE_Pressed, this, &AObserverPawn::SetModeFree);
    PlayerInputComponent->BindAction("CameraFollow", IE_Pressed, this, &AObserverPawn::SetModeFollow);

    
    PlayerInputComponent->BindAction("NextAgent", IE_Pressed, this, &AObserverPawn::NextAgent);
    PlayerInputComponent->BindAction("PreviousAgent", IE_Pressed, this, &AObserverPawn::PreviousAgent);
}

void AObserverPawn::SetModeOrbit()
{
    CurrentMode = EObserverMode::Orbit;
    UE_LOG(LogAgentDebug, Log, TEXT("Camera Mode: Orbit"));
}

void AObserverPawn::SetModeFree()
{
    CurrentMode = EObserverMode::Free;
    UE_LOG(LogAgentDebug, Log, TEXT("Camera Mode: Free"));
}

void AObserverPawn::SetModeFollow()
{
    CurrentMode = EObserverMode::Follow;
    FollowTarget = nullptr; 
    UE_LOG(LogAgentDebug, Log, TEXT("Camera Mode: Follow"));
}


void AObserverPawn::NextAgent()
{
    FollowedAgentIndex++;
}

void AObserverPawn::PreviousAgent()
{
    FollowedAgentIndex = FMath::Max(0, FollowedAgentIndex - 1);
}

void AObserverPawn::TurnCamera(float Val)
{
    
    if (CurrentMode == EObserverMode::Free && Val != 0.f)
    {
        AddControllerYawInput(Val);
    }
}

void AObserverPawn::LookUpCamera(float Val)
{
    
    if (CurrentMode == EObserverMode::Free && Val != 0.f)
    {
        AddControllerPitchInput(Val);
    }
}

