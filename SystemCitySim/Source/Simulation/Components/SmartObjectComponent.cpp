#include "Simulation/Components/SmartObjectComponent.h"
#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"

void USmartObjectComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* Owner = GetOwner())
    {
        EntryLocation = Owner->GetActorLocation(); 
    }
    
    
    
}

void USmartObjectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UWorld* World = GetWorld();

    if (IsValid(World) && !World->bIsTearingDown)
    {
        if (UInteractionSubsystem* Sub = World->GetSubsystem<UInteractionSubsystem>())
        {
            Sub->UnregisterSmartObject(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void USmartObjectComponent::InitializeSlots()
{
    Slots.Empty();

    if (!ObjectType.IsValid())
    {
        UE_LOG(LogSimulation, Error, TEXT("InitializeSlots called with INVALID ObjectType"));
        return;
    }

    FVector Base = GetOwner()->GetActorLocation();

    
    
    
    if (ObjectType == FGameplayTag::RequestGameplayTag("Type.Table"))
    {
        float Radius = 150.f;
        for (int i = 0; i < 4; i++)
        {
            float Angle = i * PI * 0.5f;
            FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);

            FInteractionSlot Slot;
            Slot.Location = Base + Offset;
            Slot.ReservedAgentID = -1;
            Slots.Add(Slot);
        }
    }
    
    
    
    else if (ObjectType == FGameplayTag::RequestGameplayTag("Type.Office"))
    {
        const int32 DeskCount = 50;
        const float Spacing = 120.f;

        for (int32 i = 0; i < DeskCount; i++)
        {
            int32 Row = i / 10;
            int32 Col = i % 10;

            FVector Offset(Row * Spacing, Col * Spacing, 0.f);

            FInteractionSlot Slot;
            Slot.Location = Base + Offset;
            Slot.ReservedAgentID = -1;
            Slots.Add(Slot);
        }
        MaxUsers = DeskCount;
    }
    
    
    
    else if (ObjectType == FGameplayTag::RequestGameplayTag("Type.Home"))
    {
        const int32 BedCount = 4;
        for (int32 i = 0; i < BedCount; i++)
        {
            FVector Offset(i * 80.f, 0.f, 0.f);
            FInteractionSlot Slot;
            Slot.Location = Base + Offset;
            Slot.ReservedAgentID = -1;
            Slots.Add(Slot);
        }
        MaxUsers = BedCount;
    }
    
    
    
    else if (ObjectType == FGameplayTag::RequestGameplayTag("Type.Cafe"))
    {
        const int32 SeatCount = 6;
        for (int32 i = 0; i < SeatCount; i++)
        {
            float Angle = i * PI * 2.f / SeatCount;
            FVector Offset(FMath::Cos(Angle) * 120.f, FMath::Sin(Angle) * 120.f, 0.f);

            FInteractionSlot Slot;
            Slot.Location = Base + Offset;
            Slot.ReservedAgentID = -1;
            Slots.Add(Slot);
        }
        MaxUsers = SeatCount;
    }
    
    
    
    else if (ObjectType == FGameplayTag::RequestGameplayTag("Type.Shop"))
    {
        MaxUsers = 6;
        const float Radius = 150.f;

        for (int i = 0; i < MaxUsers; i++)
        {
            float Angle = (2 * PI / MaxUsers) * i;
            FInteractionSlot Slot;
            Slot.Location = Base + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
            Slot.ReservedAgentID = -1;
            Slots.Add(Slot);
        }
    }
    
    
    
    else if (ObjectType == FGameplayTag::RequestGameplayTag("Type.Park"))
    {
        MaxUsers = FMath::Max(MaxUsers, 10);

        
        float GoldenAngle = 137.5f;

        for (int32 i = 0; i < MaxUsers; i++)
        {
            float Radius = 150.f + (i * 35.f); 
            float Angle = i * GoldenAngle;

            FVector Offset(
                FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
                FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
                0.f
            );

            FInteractionSlot Slot;
            Slot.Location = Base + Offset;
            Slot.ReservedAgentID = -1;
            Slots.Add(Slot);
        }
    }
}