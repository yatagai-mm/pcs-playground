#include "PCSHost.h"

#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "PCSInputHistory.h"
#include "PointCloudSequenceComponent.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailCustomization.h"
#include "Misc/MessageDialog.h"
#include "PropertyEditorModule.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
void SaveEntry(const FPCSInputHistoryEntry& Entry)
{
	UPCSInputHistory* History = GetMutableDefault<UPCSInputHistory>();
	if (History->Remember(Entry))
	{
		History->SaveConfig();
	}
}

class FPCSHistoryDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FPCSHistoryDetails>(); }

	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override
	{
		TArray<TWeakObjectPtr<UObject>> Objects;
		Builder.GetObjectsBeingCustomized(Objects);
		if (Objects.Num() != 1)
		{
			return;
		}
		const TWeakObjectPtr<UPointCloudSequenceComponent> Component = Cast<UPointCloudSequenceComponent>(Objects[0].Get());
		if (!Component.IsValid() || Component->IsTemplate())
		{
			return;
		}
		IDetailCategoryBuilder& Category = Builder.EditCategory("PCS Input History", FText::GetEmpty(), ECategoryPriority::Important);
		Category.AddCustomRow(NSLOCTEXT("PCSHost", "HistorySearch", "Recent input settings history"))
		.NameContent()[SNew(STextBlock).Text(NSLOCTEXT("PCSHost", "Recent", "Recent Settings"))]
		.ValueContent().MinDesiredWidth(250)
		[
			SNew(SComboButton)
			.OnGetMenuContent_Lambda([Component]() -> TSharedRef<SWidget>
			{
				FMenuBuilder Menu(true, nullptr);
				const UPCSInputHistory* History = GetDefault<UPCSInputHistory>();
				if (History->Entries.IsEmpty())
				{
					Menu.AddMenuEntry(NSLOCTEXT("PCSHost", "Empty", "No saved settings"), FText::GetEmpty(), FSlateIcon(),
						FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([] { return false; })));
				}
				for (int32 Index = 0; Index < History->Entries.Num(); ++Index)
				{
					const FPCSInputHistoryEntry Entry = History->Entries[Index];
					Menu.AddMenuEntry(FText::FromString(FString::Printf(TEXT("%d. %s | %s | %g fps | %g px | sRGB %s"),
						Index + 1, *Entry.Directory, *Entry.FileNameRegex, Entry.FrameRate, Entry.PointSize,
						Entry.bConvertSRGBToLinear ? TEXT("On") : TEXT("Off"))), FText::FromString(Entry.Describe()), FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([Component, Entry]
						{
							if (!Component.IsValid())
							{
								return;
							}
							if (GetMutableDefault<UPCSInputHistory>()->Apply(Entry, *Component.Get()))
							{
								SaveEntry(Entry);
							}
							else
							{
								FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("PCSHost", "RestoreFailed", "Could not restore PCS settings. The saved material may no longer exist."));
							}
						})));
				}
				return Menu.MakeWidget();
			})
			.ButtonContent()[SNew(STextBlock).Text(NSLOCTEXT("PCSHost", "Choose", "Choose recent settings (up to 5)"))]
		];
		Category.AddCustomRow(NSLOCTEXT("PCSHost", "SaveSearch", "Save current input settings"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text(NSLOCTEXT("PCSHost", "Save", "Save Current Settings"))
			.ToolTipText(NSLOCTEXT("PCSHost", "AutoSaveTip", "Settings are also saved automatically after editing. Identical settings move to the top of the history."))
			.IsEnabled_Lambda([Component] { return Component.IsValid() && FPCSInputHistoryEntry::Capture(*Component.Get()).IsValid(); })
			.OnClicked_Lambda([Component]
			{
				if (Component.IsValid())
				{
					SaveEntry(FPCSInputHistoryEntry::Capture(*Component.Get()));
				}
				return FReply::Handled();
			})
		];
	}
};
}
#endif

class FPCSHostModule final : public FDefaultGameModuleImpl
{
#if WITH_EDITOR
	struct FPendingEntry
	{
		FPCSInputHistoryEntry Entry;
		double SaveAfter = 0;
	};
	TMap<TWeakObjectPtr<UPointCloudSequenceComponent>, FPendingEntry> Pending;
	FDelegateHandle PropertyChangedHandle;
	FTSTicker::FDelegateHandle TickHandle;

	void OnPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
	{
		UPointCloudSequenceComponent* Component = Cast<UPointCloudSequenceComponent>(Object);
		if (!Component || Component->IsTemplate() || !Component->GetWorld() || Component->GetWorld()->IsGameWorld())
		{
			return;
		}
		const FName Name = Event.MemberProperty ? Event.MemberProperty->GetFName() : Event.GetPropertyName();
		static const TSet<FName> SavedProperties = {
			TEXT("SequenceDirectory"), TEXT("FrameFileNameRegex"), TEXT("FrameRate"), TEXT("PlaybackRate"),
			TEXT("bLoop"), TEXT("bAutoPlay"), TEXT("PointSize"), TEXT("bConvertSRGBToLinear"), TEXT("PointMaterial"),
			TEXT("RelativeLocation"), TEXT("RelativeRotation"), TEXT("RelativeScale3D")};
		if (SavedProperties.Contains(Name))
		{
			// Coalesce slider changes and adjacent edits instead of filling all five slots.
			Pending.Add(Component, {FPCSInputHistoryEntry::Capture(*Component), FPlatformTime::Seconds() + 1.0});
		}
	}

	bool Tick(float)
	{
		const double Now = FPlatformTime::Seconds();
		for (auto It = Pending.CreateIterator(); It; ++It)
		{
			if (Now >= It.Value().SaveAfter)
			{
				// An explicit save or restore may have superseded the queued edit.
				if (It.Key().IsValid() && FPCSInputHistoryEntry::Capture(*It.Key().Get()) == It.Value().Entry)
				{
					SaveEntry(It.Value().Entry);
				}
				It.RemoveCurrent();
			}
		}
		return true;
	}
#endif
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GetMutableDefault<UPCSInputHistory>()->Sanitize();
		FPropertyEditorModule& Editor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		Editor.RegisterCustomClassLayout("PointCloudSequenceComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FPCSHistoryDetails::MakeInstance));
		PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FPCSHostModule::OnPropertyChanged);
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FPCSHostModule::Tick));
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		for (const auto& Item : Pending)
		{
			if (Item.Key.IsValid() && FPCSInputHistoryEntry::Capture(*Item.Key.Get()) == Item.Value.Entry)
			{
				SaveEntry(Item.Value.Entry);
			}
		}
		Pending.Empty();
		if (FPropertyEditorModule* Editor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			Editor->UnregisterCustomClassLayout("PointCloudSequenceComponent");
		}
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPCSHostModule, PCSHost, "PCSHost");
