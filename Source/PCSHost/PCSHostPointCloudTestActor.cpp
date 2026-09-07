#include "PCSHostPointCloudTestActor.h"

#include "Components/SceneComponent.h"
#include "PointCloudSequenceComponent.h"

APCSHostPointCloudTestActor::APCSHostPointCloudTestActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PointCloud = CreateDefaultSubobject<UPointCloudSequenceComponent>(TEXT("PointCloudSequence"));
	PointCloud->SetupAttachment(SceneRoot);

	constexpr double PreviewScale = 100.0;
	PointCloud->SetRelativeScale3D(FVector(PreviewScale));

	PointCloud->PointSize = 1.5f;
	PointCloud->FrameRate = 5.0f;
	PointCloud->bAutoPlay = true;
	PointCloud->bLoop = true;
}
