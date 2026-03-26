
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoadActor.generated.h"

UCLASS()
class SIMULATIONSHOWCASE_API ARoadActor : public AActor
{
    GENERATED_BODY()

public:
    ARoadActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road")
    UStaticMeshComponent* RoadMeshComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road")
    UStaticMesh* RoadMesh;

    void BuildRoad(FVector StartPos, FVector EndPos);
};