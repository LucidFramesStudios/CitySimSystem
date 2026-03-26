#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Simulation/SimulationTags.h"
#include "Components/SplineComponent.h"

#include "SimulationData.generated.h"


UENUM(BlueprintType)
enum class EAgentExpression : uint8
{
    Neutral,
    Happy,
    Tired,
    Rushed,
    Confused
};
USTRUCT(BlueprintType)
struct FAgentExpressionState
{
    GENERATED_BODY()

    UPROPERTY()
    EAgentExpression CurrentExpression = EAgentExpression::Neutral;
};
UENUM(BlueprintType)
enum class EAgentState : uint8
{
    Decision,
    Traveling,
    WaitingForPath,
    Interacting,
    Cooldown,
    Idle,
    RidingBus,
    WaitingForBus,
    FinalApproach,
    Approaching,
    Reaction
};

UENUM(BlueprintType)
enum class ESimPhase : uint8 {
    Sleep,
    MorningPrep,
    Work,
    Leisure,
    WindDown
};
UENUM()
enum class EAgentType : uint8
{
    Worker,
    Civilian,
    Wanderer
};
UENUM(BlueprintType)
enum class ETransportMode : uint8
{
    Walking,
    Taxi
};
UENUM()
enum class EVehicleState : uint8
{
    Driving,
    Stopping,
    Waiting 
 

};

USTRUCT(BlueprintType)
struct FAgentPersonality
{
    GENERATED_BODY()
    UPROPERTY() float Extroversion = 0.5f;
    UPROPERTY() float CoffeeAddiction = 0.5f;
    UPROPERTY() float WorkEthic = 0.5f;
    UPROPERTY() float EnergyLevel = 0.5f;
};


USTRUCT(BlueprintType)
struct FVehicleData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 VehicleID = -1;
    
    UPROPERTY() int32 DispatchedAgentID = -1;
    UPROPERTY() int32 TaxiDestinationNode = -1;
    UPROPERTY()
    EVehicleState State = EVehicleState::Driving;

    UPROPERTY()
    FVector LogicalLocation = FVector::ZeroVector;

    UPROPERTY()
    FRotator LogicalRotation = FRotator::ZeroRotator;

    UPROPERTY()
    TArray<int32> EdgeRoute;

    UPROPERTY()
    int32 CurrentRouteIndex = 0;

    UPROPERTY()
    int32 CurrentEdgeID = -1;

    UPROPERTY()
    float DistanceAlongEdge = 0.f;

    UPROPERTY()
    float Speed = 800.f;

    UPROPERTY()
    float WaitTimer = 0.f;

    
    
    

    UPROPERTY()
    TArray<int32> Passengers;

    UPROPERTY()
    int32 MaxCapacity = 20;

    UPROPERTY() float SocialNeed = 1.0f; 
    UPROPERTY() int32 CurrentSocialTargetID = -1; 
};


UENUM(BlueprintType)
enum class ELifePhase : uint8
{
    WakeUp,
    CommuteToWork,
    Work,
    CoffeeBreak,
    Roam,
    CommuteHome,
    Relax,
    Sleep
};

USTRUCT(BlueprintType)
struct FAgentData
{
    GENERATED_BODY()
    UPROPERTY() float CommitmentTimer = 0.f;
    UPROPERTY() int32 LastVisitedSmartObject = -1;
    UPROPERTY() TMap<FGameplayTag, float> GoalCooldowns; 
    UPROPERTY() int32 AgentID = -1;
    UPROPERTY() EAgentType AgentType = EAgentType::Worker;
    UPROPERTY() FAgentPersonality Personality;
    
    
    UPROPERTY() FGameplayTag PreviousGoal;

    UPROPERTY() ETransportMode TransportMode = ETransportMode::Walking;
    UPROPERTY() int32 AssignedTaxiID = -1;
    UPROPERTY() bool bWaitingForTaxi = false;
    
    UPROPERTY() FVector LogicalLocation = FVector::ZeroVector;

    
    UPROPERTY()
    float LastDecisionTime = 0.0f;

    UPROPERTY()
    float IdleWatchdogTimer = 0.0f;

    UPROPERTY()
    int32 RetryCount = 0;


    UPROPERTY() FVector TargetLocation = FVector::ZeroVector;
    UPROPERTY() FVector FinalInsideLocation = FVector::ZeroVector;
    UPROPERTY() FVector InsideLocation = FVector::ZeroVector;
    UPROPERTY() FVector SimVelocity = FVector::ZeroVector;
    UPROPERTY() FVector PreviousLocation = FVector::ZeroVector;

    
    UPROPERTY() EAgentState State = EAgentState::Decision;
    UPROPERTY() float StateTimer = 0.f;
    UPROPERTY() bool bIsCommittedToGoal = false;
    UPROPERTY() float CommitmentLockTime = 0.f;
    UPROPERTY() float DecisionCooldown = 0.f;

    
    UPROPERTY() FGameplayTag DesiredGoal;
    UPROPERTY() FGameplayTag ActiveGoal;
    UPROPERTY() ELifePhase CurrentPhase = ELifePhase::Sleep;
    UPROPERTY() float PhaseStartTime = 0.f;
    UPROPERTY() FString DecisionReason;
    UPROPERTY() FString CurrentAnimationState = TEXT("Idle");

    
    UPROPERTY() TArray<FVector> CurrentPath;
    UPROPERTY() int32 CurrentSegmentIndex = -1;
    UPROPERTY() int32 PreviousSegmentIndex = -1;
    UPROPERTY() float SegmentProgress = 0.f;
    UPROPERTY() bool bInitializedOnSegment = false;
    UPROPERTY() int32 CurrentEdgeID = -1;
    UPROPERTY() float EdgeProgress = 0.f;

    
    UPROPERTY() int32 DesiredTargetID = -1;
    UPROPERTY() int32 ActiveInteractableID = -1;
    UPROPERTY() bool bInsideBuilding = false;
    UPROPERTY() float InteractionRemainingTime = 0.f;
    UPROPERTY() int32 HomeObjectID = -1;
    UPROPERTY() int32 WorkObjectID = -1;
    UPROPERTY() TSet<int32> FailedTargets;

    
    UPROPERTY() TMap<FGameplayTag, float> Needs;
    UPROPERTY() float WakeTime = 0.f;
    UPROPERTY() float SleepTime = 0.f;
    UPROPERTY() TArray<int32> Friends;
    UPROPERTY() FAgentExpressionState ExpressionState;

    
    UPROPERTY() int32 CurrentVehicleID = -1;
    UPROPERTY() int32 ExitBusStopID = -1;

    UPROPERTY() FRandomStream AgentSRand;
};


USTRUCT()
struct FInteractionSlot
{
    GENERATED_BODY()
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() int32 ReservedAgentID = -1;
    UPROPERTY() FGameplayTag AllowedSocialType; 
    bool IsAvailable() const { return ReservedAgentID == -1; }
};


UCLASS(BlueprintType)
class SIMULATIONSHOWCASE_API UNeedDefinition : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    FGameplayTag NeedTag;

    UPROPERTY(EditAnywhere)
    float DecayRatePerTick = 0.01f;

    UPROPERTY(EditAnywhere)
    float CriticalThreshold = 0.2f;


};

UCLASS(BlueprintType)
class SIMULATIONSHOWCASE_API  UInteractionDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    TArray<FVector> CurrentPath;
    UPROPERTY(EditAnywhere)
    FGameplayTag InteractionTag;

    UPROPERTY(EditAnywhere)
    float BaseDuration = 5.f;

    UPROPERTY(EditAnywhere)
    TMap<FGameplayTag, float> NeedModifiers;

    UPROPERTY(EditAnywhere)
    class UAnimMontage* VisualAnimation;


};