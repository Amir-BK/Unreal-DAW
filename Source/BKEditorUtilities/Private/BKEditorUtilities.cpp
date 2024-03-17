// Copyright Epic Games, Inc. All Rights Reserved.

#include "BKEditorUtilities.h"

#include "ContentBrowserModule.h"
#include "FKMidiEditorAssetActions.h"
#include "UObject/UObjectArray.h"
#include "Serialization/JsonSerializer.h"

#define LOCTEXT_NAMESPACE "BKEditorUtilitiesModule"





void BKEditorUtilitiesModule::StartupModule()
{
        //GlyphsJSON.Get()->TryGetField(TEXT("noteheadBlack")).Get()->AsObject()->TryGetField(TEXT("codepoint")).Get()->AsString();
    SFZAssetTypeActions = MakeShared<FFksfzaSampleBankAssetActions>();
    FKMidiAssetTypeActions = MakeShared<FFKMidiEditorAssetActions>();
    FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(SFZAssetTypeActions.ToSharedRef());
    //FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(FKMidiAssetTypeActions.ToSharedRef());

    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuAssetExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
    CBMenuAssetExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&BKEditorUtilitiesModule::OnExtendMidiAssetSelectionMenu));

}

void BKEditorUtilitiesModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.

    if (!FModuleManager::Get().IsModuleLoaded("AssetTools")) return;
    FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(SFZAssetTypeActions.ToSharedRef());
    FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(FKMidiAssetTypeActions.ToSharedRef());
}

void BKEditorUtilitiesModule::AddPianoRollToMidiAsset(FMenuBuilder& MenuBuilder,
    const TArray<FAssetData> SelectedAssets)
{
    for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
    {
        const FAssetData& Asset = *AssetIt;
    
        if (!Asset.IsRedirector())
        {
            
            if (Asset.GetClass()->IsChildOf(UMidiFile::StaticClass()))
            {
                
                MenuBuilder.BeginSection("BK Midi Utils", FText::FromString("BK Midi Editor"));
                {
                    MenuBuilder.AddMenuEntry(
                INVTEXT("Edit Midi File in BK Midi Editor"),
                INVTEXT("Allows modifying an existing midi asset or creating a new one within UE!"),
                FSlateIcon(),//FLinterStyle::GetStyleSetName(), "Linter.Toolbar.Icon"),
                FUIAction(FExecuteAction::CreateLambda([Asset]()
                {
                        // Do work here.
                    OpenSelectedMidiFileInEditorWidget(Asset.GetSoftObjectPath());
                })),
                NAME_None,
                EUserInterfaceActionType::Button);
                }
                MenuBuilder.EndSection();
            }
        }
    }

}

TSharedRef<FExtender> BKEditorUtilitiesModule::OnExtendMidiAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
    TSharedRef<FExtender> Extender = MakeShared<FExtender>();
    Extender->AddMenuExtension(
        "CommonAssetActions",
        EExtensionHook::Before,
        nullptr,
        FMenuExtensionDelegate::CreateStatic(&BKEditorUtilitiesModule::AddPianoRollToMidiAsset, SelectedAssets)
    );
    return Extender;
}

void BKEditorUtilitiesModule::OpenSelectedMidiFileInEditorWidget(FSoftObjectPath MidiFileSoftPath)
{
    //OpenSelectedMidiFileInEditorWidget(MidiFileSoftPath);
    UObject* MidiFileObject = MidiFileSoftPath.TryLoad();
    if(MidiFileObject != nullptr)
    {
        UMidiFile* ObjectAsMidi = Cast<UMidiFile>(MidiFileObject);
        if(ObjectAsMidi != nullptr)
        {
            BKEditorUtilitiesModule::OpenSelectedMidiFileInEditorWidget(ObjectAsMidi);
        }
    }
}

void BKEditorUtilitiesModule::OpenSelectedMidiFileInEditorWidget(UMidiFile* MidiFilePointer)
{

        const FSoftObjectPath widgetAssetPath("/Script/Blutility.EditorUtilityWidgetBlueprint'/unDAW/EditorWidget/PianoRoll/BK_MidiPianoRoll.BK_MidiPianoRoll'");

        UObject* widgetAssetLoaded = widgetAssetPath.TryLoad();
        if (widgetAssetLoaded == nullptr) {
            UE_LOG(LogTemp, Warning, TEXT("Missing Expected widget class at : /LevelValidationTools/EUW_LevelValidator.EUW_LevelValidator"));
            return;
        }

        UEditorUtilityWidgetBlueprint* widget = Cast<UEditorUtilityWidgetBlueprint>(widgetAssetLoaded);

        if (widget == nullptr) {
            UE_LOG(LogTemp, Warning, TEXT("Couldnt cast /LevelValidationTools/EUW_LevelValidator.EUW_LevelValidator to UEditorUtilityWidgetBlueprint"));
            return;
        }


        UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
        auto spawnedWidget = EditorUtilitySubsystem->SpawnAndRegisterTab(widget);
        
        auto EngPanel = spawnedWidget->WidgetTree.Get()->FindWidget(FName(TEXT("MIDIEditorBase")));
        UMIDIEditorBase* widgetAsEngraving = Cast<UMIDIEditorBase>(EngPanel);
        if (widgetAsEngraving == nullptr) {
            //spawnedWidget->GetRootWidget()->Slot.Get().
            UE_LOG(LogTemp, Warning, TEXT("Couldnt cast to base CPP class"));
            return;
        }
    

        widgetAsEngraving->HarmonixMidiFile = MidiFilePointer;
        widgetAsEngraving->InitFromDataHarmonix();

    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(BKEditorUtilitiesModule, BKEditorUtilities)

