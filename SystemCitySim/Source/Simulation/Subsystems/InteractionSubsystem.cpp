#include "Simulation/Subsystems/InteractionSubsystem.h"
#include "Simulation/Subsystems/NeedsSubsystem.h"
#include "Simulation/Logging/SimulationLog.h"
#include "Simulation/Subsystems/SimulationRandomSubsystem.h"
#include "Simulation/Subsystems/TransitSubsystem.h"
#include "Simulation/Subsystems/VehicleSimulationSubsystem.h"
#include "Simulation/Subsystems/WorldSimulationSubsystem.h"
#include "Simulation/Subsystems/PathfindingSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Simulation/Components/SmartObjectComponent.h"
#include "Simulation/SimulationTags.h"
#include "Simulation/Subsystems/RoadPathfindingSubsystem.h"
#include "Simulation/Data/SimulationData.h"

FIntVector UInteractionSubsystem::GetCellCoords(const FVector& Location) const
{
    return FIntVector(
        FMath::FloorToInt(Location.X / CellSize),
        FMath::FloorToInt(Location.Y / CellSize),
        0
    );
}

void UInteractionSubsystem::RebuildIndexMap()
{
    ObjectIDToIndex.Empty();

    for (int32 i = 0; i < SmartObjects.Num(); i++)
    {
        ObjectIDToIndex.Add(SmartObjects[i].ObjectID, i);
    }
}

void UInteractionSubsystem::RegisterSmartObject(USmartObjectComponent* Object)
{

    UE_LOG(LogSimulation, Warning,
        TEXT("REGISTER ATTEMPT: Owner=%s Type=%s Slots=%d"),
        *Object->GetOwner()->GetName(),
        *Object->ObjectType.ToString(),
        Object->Slots.Num());


    if (!Object)
        return;

    
    
    
    if (!Object->ObjectType.IsValid())
    {
        UE_LOG(LogSimulation, Error,
            TEXT("SmartObject registration FAILED: Invalid Type"));
        return;
    }

    
    
    
    FSmartObjectEntry Entry;
     
    Entry.ObjectID = SmartObjects.Num() + 1;
    Entry.Location = Object->GetOwner()->GetActorLocation();
    Entry.Component = Object;
    Entry.EntryLocation = Object->EntryLocation;
    Entry.InsideLocation = Object->InsideLocation;
    Entry.InsideLocation.Z = 0.0f;
    Object->ObjectID = Entry.ObjectID;

    int32 Index = SmartObjects.Add(Entry);
    ObjectIDToIndex.Add(Entry.ObjectID, Index);
    if (URoadPathfindingSubsystem* RoadPath = GetWorld()->GetSubsystem<URoadPathfindingSubsystem>())
    {
        int32 ClosestNode = RoadPath->GetNearestNode(Entry.Location);
        UE_LOG(LogSimulation, Warning, TEXT("[BUILDING] ID=%d -> NearestNode=%d"), Entry.ObjectID, ClosestNode);
    }
    
    
    
    FIntVector Cell = GetCellCoords(Entry.Location);
    FSpatialCell& GridCell = SpatialGrid.FindOrAdd(Cell);

    GridCell.ObjectIDs.AddUnique(Entry.ObjectID);

    
    
    
    UE_LOG(LogSimulation, Warning,
        TEXT("SmartObject Registered ID=%d Type=%s Cell(%d,%d) Total=%d"),
        Entry.ObjectID,
        *Object->ObjectType.ToString(),
        Cell.X,
        Cell.Y,
        SmartObjects.Num());

    UE_LOG(LogSimulation, Warning,
        TEXT("DETAIL: Type=%s Slots=%d MaxUsers=%d"),
        *Object->ObjectType.ToString(),
        Object->Slots.Num(),
        Object->MaxUsers);
}



FSmartObjectEntry* UInteractionSubsystem::GetSmartObjectByID(int32 ObjectID)
{
    if (int32* Index = ObjectIDToIndex.Find(ObjectID))
    {
        if (SmartObjects.IsValidIndex(*Index))
        {
            return &SmartObjects[*Index];
        }
    }

    return nullptr;
}

UInteractionDefinition* UInteractionSubsystem::GetInteractionDefinition(FGameplayTag Tag)
{
    return RegisteredInteractionDefinitions.FindRef(Tag);
}
void UInteractionSubsystem::ResolveTimerCompletion(FAgentData& Agent)
{
    
    ApplyInteractionEffects(Agent);

    
    if (Agent.ActiveInteractableID != -1)
    {
        ReleaseSlot(Agent.ActiveInteractableID, Agent.AgentID);

        
        Agent.FailedTargets.Empty();   // reset the blacklist on a successful interaction (don't blacklist the venue we just used)
    }

    Agent.bInsideBuilding = false;

    

    
    Agent.ActiveInteractableID = -1;
    Agent.DesiredGoal = FGameplayTag::EmptyTag;

    
    Agent.State = EAgentState::Cooldown;
    Agent.StateTimer = 1.0f;
}

void UInteractionSubsystem::ApplyInteractionEffects(FAgentData& Agent)
{
    UInteractionDefinition* Def =
        GetInteractionDefinition(Agent.DesiredGoal);

    if (!Def)
        return;

    for (const auto& Modifier : Def->NeedModifiers)
    {
        if (float* Need = Agent.Needs.Find(Modifier.Key))
        {
            *Need = FMath::Clamp(
                *Need + Modifier.Value,
                0.f,
                1.f);
        }
    }
}

void UInteractionSubsystem::ProcessTick(float DeltaTime)
{
    UpdateAgentDensity();
}

void UInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    NeighborOffsets.Empty();

    for (int32 X = -1; X <= 1; X++)
    {
        for (int32 Y = -1; Y <= 1; Y++)
        {
            NeighborOffsets.Add(FIntVector(X, Y, 0));
        }
    }

    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> Assets;

    AssetRegistryModule.Get().GetAssetsByClass(
        UInteractionDefinition::StaticClass()->GetClassPathName(),
        Assets
    );

    for (const FAssetData& Asset : Assets)
    {
        if (UInteractionDefinition* Def =
            Cast<UInteractionDefinition>(Asset.GetAsset()))
        {
            RegisteredInteractionDefinitions.Add(
                Def->InteractionTag,
                Def);

            UE_LOG(LogSimulation, Log,
                TEXT("Registered InteractionDefinition: %s"),
                *Def->InteractionTag.ToString());
        }
    }
}

FInteractionSlot* UInteractionSubsystem::ReserveSlot(int32 ObjectID, int32 AgentID)
{
    FSmartObjectEntry* Obj = GetSmartObjectByID(ObjectID);
    if (!Obj || !Obj->Component) return nullptr;

    if (Obj->Component->CurrentUsers >= Obj->Component->MaxUsers)
    {
        return nullptr; 
    }

    for (int32 i = 0; i < Obj->Component->Slots.Num(); i++)
    {
        FInteractionSlot& Slot = Obj->Component->Slots[i];
        if (Slot.IsAvailable())
        {
            Slot.ReservedAgentID = AgentID;
            Obj->Component->CurrentUsers++;

            UE_LOG(LogSimulation, Log, TEXT("[Agent %d] SLOT RESERVED -> Object %d (%d/%d)"),
                AgentID, ObjectID, Obj->Component->CurrentUsers, Obj->Component->MaxUsers);

            return &Slot;
        }
    }
    return nullptr;
}

void UInteractionSubsystem::ReleaseSlot(int32 ObjectID, int32 AgentID)
{
    FSmartObjectEntry* Obj = GetSmartObjectByID(ObjectID);
    if (!Obj || !Obj->Component) return;

    for (int32 i = 0; i < Obj->Component->Slots.Num(); i++)
    {
        FInteractionSlot& Slot = Obj->Component->Slots[i];
        if (Slot.ReservedAgentID == AgentID)
        {
            Slot.ReservedAgentID = -1;
            Obj->Component->CurrentUsers = FMath::Max(0, Obj->Component->CurrentUsers - 1);

            UE_LOG(LogSimulation, Log, TEXT("[Agent %d] SLOT RELEASED -> Object %d"), AgentID, ObjectID);
            return;
        }
    }
}

 

void UInteractionSubsystem::Sim_DebugSpatialGrid()
{
    for (const auto& Cell : SpatialGrid)
    {
        UE_LOG(LogSimulation, Log,
            TEXT("Cell(%d,%d) Objects=%d"),
            Cell.Key.X,
            Cell.Key.Y,
            Cell.Value.ObjectIDs.Num());
    }
}

void UInteractionSubsystem::QueryNearbyObjects(
    const FVector& Location,
    TArray<FSmartObjectEntry*>& OutObjects)
{
    OutObjects.Reset();

    FIntVector CenterCell = GetCellCoords(Location);

    for (const FIntVector& Offset : NeighborOffsets)
    {
        FIntVector Target =
            CenterCell + Offset;

        if (const FSpatialCell* Cell =
            SpatialGrid.Find(Target))
        {
            for (int32 ID : Cell->ObjectIDs)
            {
                if (FSmartObjectEntry* Entry =
                    GetSmartObjectByID(ID))
                {
                    OutObjects.Add(Entry);
                }
            }
        }
    }
}



FVector UInteractionSubsystem::GetObjectLocation(int32 ObjectID) const
{
    for (const FSmartObjectEntry& Entry : SmartObjects)
    {
        if (Entry.ObjectID == ObjectID)
        {
            return Entry.Location;
        }
    }

    return FVector::ZeroVector;
}

// Utility scoring function: Inversely proportional to distance squared, penalized by current occupancy. ObjectID modulo acts as a deterministic tie-breaker to distribute agents evenly across equidistant valid targets
int32 UInteractionSubsystem::FindBestObjectForGoal(FGameplayTag Goal, const FVector& AgentLocation, const TSet<int32>& FailedTargets, int32 CurrentObjectID) const
{
    float BestScore = -MAX_FLT;
    int32 BestID = -1;

    for (const FSmartObjectEntry& Entry : SmartObjects)
    {
        USmartObjectComponent* Obj = Entry.Component;
        if (!Obj || Obj->ObjectType != Goal || FailedTargets.Contains(Obj->ObjectID)) continue;  

            
            float DistSq = FVector::DistSquared(AgentLocation, Entry.Location);

        
        float Score = (10000000.f / (DistSq + 1.f));

        
        Score += (float)(Entry.ObjectID % 10) * 100.f;

        
        float Occupancy = (float)Obj->CurrentUsers / (float)Obj->MaxUsers;
        Score *= (1.0f - (Occupancy * 0.5f));

        if (Score > BestScore)
        {
            BestScore = Score;
            BestID = Obj->ObjectID;  
        }
    }

    if (BestID == -1)
    {
        // Blacklist saturated (e.g. the synchronized coffee rush): fall back to the
        // nearest matching object IGNORING FailedTargets, so the agent still walks to
        // a venue and waits instead of dropping into the wandering failsafe.
        float FallbackDistSq = MAX_FLT;
        for (const FSmartObjectEntry& Entry : SmartObjects)
        {
            USmartObjectComponent* Obj = Entry.Component;
            if (!Obj || Obj->ObjectType != Goal) continue;
            const float D = FVector::DistSquared(AgentLocation, Entry.Location);
            if (D < FallbackDistSq) { FallbackDistSq = D; BestID = Obj->ObjectID; }
        }
    }
    return BestID;
}

FVector UInteractionSubsystem::GetObjectEntryLocation(int32 ObjectID) const
{
    const int32* Index = ObjectIDToIndex.Find(ObjectID);
    if (Index)
    {
        return SmartObjects[*Index].EntryLocation;
    }

    return FVector::ZeroVector;
}

FVector UInteractionSubsystem::GetInsideLocation(int32 ObjectID) const
{
    const int32* Index = ObjectIDToIndex.Find(ObjectID);
    if (Index)
    {
        return SmartObjects[*Index].InsideLocation;
    }

    return FVector::ZeroVector;
}
const USmartObjectComponent* UInteractionSubsystem::GetSmartObject(int32 ObjectID) const
{
    if (const int32* Index = ObjectIDToIndex.Find(ObjectID))
    {
        if (SmartObjects.IsValidIndex(*Index))
        {
            return SmartObjects[*Index].Component;
        }
    }

    SIM_LOG_ERROR(-1, "GetSmartObject FAILED: ObjectID %d not found in index.", ObjectID);
    return nullptr;
}



void UInteractionSubsystem::ProcessAgentInteraction(FAgentData& Agent, float DeltaTime)
{
    if (!Agent.bInsideBuilding || Agent.State != EAgentState::Interacting) return;

    Agent.InteractionRemainingTime -= DeltaTime;
    
    Agent.SimVelocity = FVector::ZeroVector;

    
    bool bPhaseRequiresHold = false;
    if (Agent.CurrentPhase == ELifePhase::Work && Agent.ActiveGoal == FSimTags::Type_Office) bPhaseRequiresHold = true;

    
    if ((Agent.CurrentPhase == ELifePhase::Sleep ||
        Agent.CurrentPhase == ELifePhase::CommuteHome ||
        Agent.CurrentPhase == ELifePhase::WakeUp) &&
        Agent.ActiveGoal == FSimTags::Type_Home)
    {
        bPhaseRequiresHold = true;
    }

    
    int32 MicroCycle = (FMath::FloorToInt(Agent.InteractionRemainingTime) + (Agent.AgentID * 7)) % 4;
    FString NewAnim = TEXT("Idle");

    if (Agent.ActiveGoal == FSimTags::Type_Park || Agent.ActiveGoal == FSimTags::Type_Cafe)
    {
        bool bIsSocializing = false;
        UNeedsSubsystem* Needs = GetWorld()->GetSubsystem<UNeedsSubsystem>();
        if (Needs)
        {
            for (const FAgentData& Other : Needs->GetActiveAgents())
            {
                
                if (Other.AgentID != Agent.AgentID && Other.bInsideBuilding && Other.ActiveInteractableID == Agent.ActiveInteractableID)
                {
                    bIsSocializing = true;
                    
                    break;
                }
            }
        }

        if (bIsSocializing) NewAnim = TEXT("Talking");
        else if (Agent.ActiveGoal == FSimTags::Type_Cafe) NewAnim = (MicroCycle == 0) ? TEXT("Sitting") : TEXT("Drinking");
        else NewAnim = (MicroCycle == 0) ? TEXT("Walking") : TEXT("Relaxing");
    }
    else if (Agent.ActiveGoal == FSimTags::Type_Office)
    {
        if (MicroCycle == 0) NewAnim = TEXT("Working");
        else if (MicroCycle == 1) NewAnim = TEXT("Talking");
        else if (MicroCycle == 2) NewAnim = TEXT("Writing");
        else NewAnim = TEXT("Printing");
    }
    else if (Agent.ActiveGoal == FSimTags::Type_Shop)
    {
        if (MicroCycle == 0) NewAnim = TEXT("Browsing");
        else if (MicroCycle == 1) NewAnim = TEXT("PickingItem");
        else if (MicroCycle == 2) NewAnim = TEXT("Paying");
        else NewAnim = TEXT("Idle");
    }

    if (Agent.CurrentAnimationState != NewAnim)
    {
        Agent.CurrentAnimationState = NewAnim;
    }

    
    if (Agent.InteractionRemainingTime <= 0.f)
    {
        if (bPhaseRequiresHold)
        {
            Agent.InteractionRemainingTime = 5.0f; 
            
        }
        else
        {
            ResolveTimerCompletion(Agent);
        }
    }
}

void UInteractionSubsystem::UnregisterSmartObject(USmartObjectComponent* Object)
{
    
    
    if (!Object) return;
    int32 TargetID = Object->ObjectID;

    SmartObjects.RemoveAll([TargetID](const FSmartObjectEntry& Entry)
        {
            return Entry.ObjectID == TargetID;
        });

    RebuildIndexMap();

    FVector Location = Object->GetOwner()->GetActorLocation();
    FIntVector Cell = GetCellCoords(Location);
    if (FSpatialCell* GridCell = SpatialGrid.Find(Cell))
    {
        GridCell->ObjectIDs.Remove(TargetID);
        if (GridCell->ObjectIDs.Num() == 0) SpatialGrid.Remove(Cell);
    }
}
// Spatial Partitioning: Updates 2D grid cell density based on logical agent coordinates. Offloads O(N) distance checks for localized queries.
void UInteractionSubsystem::UpdateAgentDensity()
{
    UNeedsSubsystem* Needs = GetWorld()->GetSubsystem<UNeedsSubsystem>();
    if (!Needs) return;

    
    for (auto& Pair : SpatialGrid)
    {
        Pair.Value.AgentCount = 0;
    }

    
    for (const FAgentData& Agent : Needs->GetActiveAgents())
    {
        FIntVector Cell = GetCellCoords(Agent.LogicalLocation);

        FSpatialCell& CellData = SpatialGrid.FindOrAdd(Cell);
        CellData.AgentCount++;
    }
}