#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PCSHostPointCloudTestActor.generated.h"

class UPointCloudSequenceComponent;
class USceneComponent;

// Development-only host actor used to exercise the PCS render path.
UCLASS()
class PCSHOST_API APCSHostPointCloudTestActor final : public AActor
{
	GENERATED_BODY()

public:
	APCSHostPointCloudTestActor();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointCloudSequenceComponent> PointCloud;
};
