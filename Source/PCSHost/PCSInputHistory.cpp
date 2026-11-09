#include "PCSInputHistory.h"

#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "PointCloudSequenceComponent.h"
#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

FPCSInputHistoryEntry FPCSInputHistoryEntry::Capture(const UPointCloudSequenceComponent& Component)
{
	FPCSInputHistoryEntry Entry;
	Entry.Directory = Component.GetSequenceDirectory();
	Entry.FileNameRegex = Component.GetFrameFileNameRegex();
	Entry.FrameRate = Component.FrameRate;
	Entry.PlaybackRate = Component.PlaybackRate;
	Entry.bLoop = Component.bLoop;
	Entry.bAutoPlay = Component.bAutoPlay;
	Entry.PointSize = Component.PointSize;
	Entry.bConvertSRGBToLinear = Component.bConvertSRGBToLinear;
	Entry.Material = FSoftObjectPath(Component.PointMaterial.Get());
	Entry.RelativeTransform = Component.GetRelativeTransform();
	Entry.Normalize();
	return Entry;
}

void FPCSInputHistoryEntry::Normalize()
{
	if (!Directory.IsEmpty())
	{
		Directory = FPaths::ConvertRelativePathToFull(Directory);
		FPaths::NormalizeDirectoryName(Directory);
		FPaths::CollapseRelativeDirectories(Directory);
	}
}

bool FPCSInputHistoryEntry::IsValid() const
{
	return !Directory.IsEmpty() && !FileNameRegex.IsEmpty() &&
		FMath::IsFinite(FrameRate) && FrameRate >= 1.0f &&
		FMath::IsFinite(PlaybackRate) && PlaybackRate >= 0.0f &&
		FMath::IsFinite(PointSize) && PointSize >= 0.1f && !RelativeTransform.ContainsNaN();
}

bool FPCSInputHistoryEntry::operator==(const FPCSInputHistoryEntry& Other) const
{
	return Directory == Other.Directory && FileNameRegex == Other.FileNameRegex &&
		FrameRate == Other.FrameRate && PlaybackRate == Other.PlaybackRate &&
		bLoop == Other.bLoop && bAutoPlay == Other.bAutoPlay && PointSize == Other.PointSize &&
		bConvertSRGBToLinear == Other.bConvertSRGBToLinear && Material == Other.Material &&
		// Config serialization and component Euler/quaternion conversion introduce rounding.
		RelativeTransform.Equals(Other.RelativeTransform, UE_KINDA_SMALL_NUMBER);
}

FString FPCSInputHistoryEntry::Describe() const
{
	return FString::Printf(TEXT("%s\n%s\nFPS: %g | Speed: %g | Point size: %g | sRGB: %s | Loop: %s | Auto play: %s\nMaterial: %s\nTransform: %s"),
		*Directory, *FileNameRegex, FrameRate, PlaybackRate, PointSize,
		bConvertSRGBToLinear ? TEXT("On") : TEXT("Off"), bLoop ? TEXT("On") : TEXT("Off"),
		bAutoPlay ? TEXT("On") : TEXT("Off"), *Material.ToString(), *RelativeTransform.ToHumanReadableString());
}

bool UPCSInputHistory::Remember(FPCSInputHistoryEntry Entry)
{
	Entry.Normalize();
	if (!Entry.IsValid())
	{
		return false;
	}
	const TArray<FPCSInputHistoryEntry> Previous = Entries;
	Entries.RemoveAll([&Entry](const FPCSInputHistoryEntry& Existing) { return Existing == Entry; });
	Entries.Insert(MoveTemp(Entry), 0);
	Sanitize();
	return Entries != Previous;
}

void UPCSInputHistory::Sanitize()
{
	TArray<FPCSInputHistoryEntry> Clean;
	for (FPCSInputHistoryEntry Entry : Entries)
	{
		Entry.Normalize();
		if (Entry.IsValid() && !Clean.Contains(Entry))
		{
			Clean.Add(MoveTemp(Entry));
			if (Clean.Num() == MaxEntries)
			{
				break;
			}
		}
	}
	Entries = MoveTemp(Clean);
}

#if WITH_EDITOR
bool UPCSInputHistory::Apply(const FPCSInputHistoryEntry& Entry, UPointCloudSequenceComponent& Component)
{
	if (!Entry.IsValid())
	{
		return false;
	}
	UMaterialInterface* Material = Cast<UMaterialInterface>(Entry.Material.TryLoad());
	if (!Entry.Material.IsNull() && Material == nullptr)
	{
		return false;
	}
	const FScopedTransaction Transaction(NSLOCTEXT("PCSHost", "RestoreHistory", "Restore PCS input settings"));
	Component.Modify();
	Component.FrameRate = Entry.FrameRate;
	Component.PlaybackRate = Entry.PlaybackRate;
	Component.bLoop = Entry.bLoop;
	Component.bAutoPlay = Entry.bAutoPlay;
	Component.PointSize = Entry.PointSize;
	Component.SetConvertSRGBToLinear(Entry.bConvertSRGBToLinear);
	Component.PointMaterial = Material;
	Component.SetRelativeTransform(Entry.RelativeTransform);
	Component.SetSequenceSource(Entry.Directory, Entry.FileNameRegex);
	Component.MarkRenderStateDirty();
	Component.MarkPackageDirty();
	return true;
}
#endif
