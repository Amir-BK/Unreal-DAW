
#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditorUndoClient.h"
#include "SequencerData.h"
#include "SPianoRollGraph.h"
#include "HarmonixMetasound/DataTypes/MusicTimestamp.h"

#include "GlyphButton.h"

#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "GraphEditor.h"


class FSequenceAssetDetails : public IDetailCustomization
{
public:
    // This function will be called when the properties are being customized
    void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

    static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShareable(new FSequenceAssetDetails()); }
private:
    UDAWSequencerData* SequenceData = nullptr;

    void OnFusionPatchChangedInTrack(int TrackID, UFusionPatch* NewPatch);

    TSharedPtr<SVerticalBox> MidiInputTracks;

    void UpdateMidiInputTracks();

    // This function will create a new instance of this class as a shared pointer
};


class FUnDAWSequenceEditorToolkit : public FAssetEditorToolkit, public IBK_MusicSceneManagerInterface, public FEditorUndoClient
{
public:
    void InitEditor(const TArray<UObject*>& InObjects);
 
    void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
    void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

    TSharedRef<SButton> GetConfiguredTransportButton(EBKTransportCommands InCommand);

    void OnSelectionChanged(const TSet<UObject*>& SelectedNodes);

    void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);
 
    FName GetToolkitFName() const override { return "unDAWSequenceEditor"; }
    FText GetBaseToolkitName() const override { return INVTEXT("unDAW Sequence Editor"); }
    FString GetWorldCentricTabPrefix() const override { return "unDAW "; }
    FLinearColor GetWorldCentricTabColorScale() const override { return {}; }

    void CreateGraphEditorWidget();

    TSharedPtr<SDockTab> MetasoundGraphEditorBox;
    TSharedPtr<SGraphEditor> MetasoundGraphEditor;


    void ExtendToolbar();

    ~FUnDAWSequenceEditorToolkit();

protected:
    UDAWSequencerData* SequenceData;
    TSharedPtr<FUICommandList> AdditionalGraphCommands;
    TSharedPtr<SPianoRollGraph> PianoRollGraph;

    TSharedPtr<STextBlock> CurrentPlayStateTextBox;
    TSharedPtr<SHorizontalBox> TransportControls;

    void SetupPreviewPerformer();
    //void PlayAudioComponent();
    //void StopAudioComponent();

    //UAudioComponent* AudioComponent = nullptr;
};
