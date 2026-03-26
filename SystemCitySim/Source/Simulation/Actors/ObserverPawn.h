#pragma once

#include "CoreMinimal.h"
#include "Simulation/Subsystems/CityEventSubsystem.h"
#include "GameFramework/Pawn.h"
#include "ObserverPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class AVisualAgentActor;

UENUM()
enum class EObserverMode : uint8
{
    Orbit,
    Free,
    Follow
};

UCLASS()
class SIMULATIONSHOWCASE_API AObserverPawn : public APawn
{
    GENERATED_BODY()

public:
    AObserverPawn();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    UFUNCTION(BlueprintCallable)
    void SetModeOrbit();
    UFUNCTION(BlueprintCallable)
    void TurnCamera(float Val);
    UFUNCTION(BlueprintCallable)
    void LookUpCamera(float Val);
    UFUNCTION(BlueprintCallable)
    void SetModeFree();

    UFUNCTION(BlueprintCallable)
    void SetModeFollow();
    FVector FollowOffsetInput = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere,  BlueprintReadWrite)
    class USpringArmComponent* SpringArmComp;
    UFUNCTION(BlueprintCallable)
    void SetTimeScale1x();
    UFUNCTION(BlueprintCallable)
    void SetTimeScale5x();
    UFUNCTION(BlueprintCallable)
    void SetTimeScale20x();
    UFUNCTION(BlueprintCallable)
    void SetTimeScale(float NewScale);
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UCameraComponent* CameraComp;

    UPROPERTY(EditAnywhere)
    UFloatingPawnMovement* MovementComp;

    UPROPERTY()
    AVisualAgentActor* SelectedAgent;

    
    UPROPERTY()
    AActor* FollowTarget = nullptr;

    EObserverMode CurrentMode = EObserverMode::Orbit;

    int32 FollowedAgentIndex = 0;
    UFUNCTION(BlueprintCallable)
    void NextAgent();
    UFUNCTION(BlueprintCallable)
    void PreviousAgent();
    UFUNCTION(BlueprintCallable)
    void ToggleRain();

    UFUNCTION(BlueprintCallable)
    void ToggleRushHour();
private:
    void MoveForward(float Val);
    void MoveRight(float Val);
    void MoveUp(float Val);

     
    
    void ToggleFestival();
    void TriggerEvent(ECityEvent EventType);
    void ToggleHUD();
    void TrySelectAgent();
    void ClearSelection();

    
     
};