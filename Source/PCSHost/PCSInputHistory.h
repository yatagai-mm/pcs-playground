#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PCSInputHistory.generated.h"

class UPointCloudSequenceComponent;

USTRUCT()
struct FPCSInputHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY() FString Directory;
	UPROPERTY() FString FileNameRegex;
	UPROPERTY() float FrameRate = 30.0f;
	UPROPERTY() float PlaybackRate = 1.0f;
	UPROPERTY() bool bLoop = true;
	UPROPERTY() bool bAutoPlay = true;
	UPROPERTY() float PointSize = 1.0f;
	UPROPERTY() bool bConvertSRGBToLinear = true;
	UPROPERTY() FSoftObjectPath Material;
	UPROPERTY() FTransform RelativeTransform = FTransform::Identity;

	static FPCSInputHistoryEntry Capture(const UPointCloudSequenceComponent& Component);
	void Normalize();
	bool IsValid() const;
	bool operator==(const FPCSInputHistoryEntry& Other) const;
	FString Describe() const;
};

// Local to this project and user; never serialized into a level or the PCS plugin.
UCLASS(Config = EditorPerProjectUserSettings)
class UPCSInputHistory : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxEntries = 5;

	UPROPERTY(Config) TArray<FPCSInputHistoryEntry> Entries;

	// Returns whether the history changed. Call SaveConfig only when it did.
	bool Remember(FPCSInputHistoryEntry Entry);
	void Sanitize();

#if WITH_EDITOR
	bool Apply(const FPCSInputHistoryEntry& Entry, UPointCloudSequenceComponent& Component);
#endif
};
