// Fill out your copyright notice in the Description page of Project Settings.


#include "MIDIEditorBase.h"
#include <Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h>
#include "Widgets/Colors/SColorWheel.h"
#include "Widgets/Colors/SColorSpectrum.h"
#include "Widgets/Colors/SColorBlock.h"
#include <Customizations/ColorStructCustomization.h>
#include "Quartz/AudioMixerClockHandle.h"
#include "SEnumCombo.h"
#include <Quartz/QuartzSubsystem.h>

#include "SMidiTrackControlsWidget.h"

DEFINE_LOG_CATEGORY(BKMidiLogs);

#define TRANSACT(Name) 	if (GEditor && GEditor->CanTransact() && ensure(!GIsTransacting)) \
GEditor->BeginTransaction(TEXT(""), INVTEXT(Name), nullptr);


struct FEventsWithIndex
{
	FMidiEvent event;
	int32 eventIndex;
};

void UMIDIEditorBase::SetCurrentTimelinePosition(float inPosition)
{
	
	UE_LOG(BKMidiLogs, Verbose, TEXT("Received Seek Request %f"), inPosition);
	
	if (currentTimelineCursorPosition == inPosition) return;
	
	if (SceneManager)
	{
		UE_LOG(BKMidiLogs, Verbose, TEXT("We have a scene manager... %f"), inPosition);
		SceneManager->SendSeekCommand(inPosition);

	}
	else {
		//SeekEventDelegate.Broadcast(inPosition);
		currentTimelineCursorPosition = inPosition;
	}

	if (TransportWidget)
	{
		TransportWidget->SetTransportSeek(inPosition);
	}
	
	
	for (const auto& graph : InternalGraphs)
	{
		graph->CurrentTimelinePosition = inPosition;
	}
	
}

void UMIDIEditorBase::AddDeltaToTimeLine(float inDelta)
{
	currentTimelineCursorPosition += inDelta;
	for (const auto& graph : InternalGraphs)
	{
		graph->CurrentTimelinePosition += inDelta;
	}
}

float UMIDIEditorBase::getCurrentTimelinePosition()
{
	return currentTimelineCursorPosition;
}

TSharedRef<SWidget> UMIDIEditorBase::RebuildWidget()
{
	InvalidTrackRef = FTrackDisplayOptions();
	InvalidTrackRef.trackColor = FLinearColor::Gray;
	
	if (TransportWidget)
	{
		TransportWidget->TransportCalled.AddUniqueDynamic(this, &UMIDIEditorBase::ReceiveTransportCommand);
		TransportWidget->TransportSeekCommand.AddUniqueDynamic(this, &UMIDIEditorBase::SetCurrentPosition);
	}
	
	tracksVerticalBox = SNew(SVerticalBox);

	
	panelMainScrollArea = SNew(SScrollBox)
		.Orientation(EOrientation::Orient_Vertical)
		.AllowOverscroll(EAllowOverscroll::Yes)
		.ScrollBarAlwaysVisible(false)

		.RenderTransform(FSlateRenderTransform(FVector2f(1.0f, 4.0f)));


	return panelMainScrollArea.ToSharedRef();
}

void UMIDIEditorBase::ReleaseSlateResources(bool bReleaseChildren)
{
	
	

	//for (auto graph : InternalGraphs) graph->ResetCanvas();
	
	tracksVerticalBox.Reset();
	panelMainScrollArea.Reset();
	
	MidiEditorSharedPtr.Reset();

	Super::ReleaseSlateResources(true);
	

	
}

FOnPlaybackStateChanged* UMIDIEditorBase::GetPlaybackStateDelegate()
{
	return &PlaybackStateDelegate;
}

FOnTransportSeekCommand* UMIDIEditorBase::GetSeekCommandDelegate()
{
	return &SeekEventDelegate;
}

UAudioComponent* UMIDIEditorBase::GetAudioComponent()
{
	if (SceneManager && SceneManager != this)
	{
		return SceneManager->GetAudioComponent();
	}
	
	if (!PerformanceComponent)
	{
	//PerformanceComponent = UGameplayStatics::CreateSound2D(WorldContextObject, nullptr, 1.0f, 1.0f, 0.0f);
	}
	

	return PerformanceComponent;
}

void UMIDIEditorBase::SetPlaybackState(EBKPlayState newPlayState)
{
	UE_LOG(BKMidiLogs, Log, TEXT("Received set playbackstate in MidiEditor, %s"), *UEnum::GetValueAsString(newPlayState))
		if (SceneManager) { SceneManager->SetPlaybackState(newPlayState); }
		else {
			UE_LOG(BKMidiLogs, Log, TEXT("No Scene Manager, %s"), *UEnum::GetValueAsString(newPlayState))
		}

}

UDAWSequencerData* UMIDIEditorBase::GetActiveSessionData()
{
	if (SceneManager && SceneManager != this)
	{
		return SceneManager->GetActiveSessionData();
		
	}
	return PreviewCache.Get();
}

TSubclassOf<UMetasoundBuilderHelperBase> UMIDIEditorBase::GetBuilderBPClass()
{
	if (SceneManager != this) return SceneManager->GetBuilderBPClass();
	
	return BuilderBPInstance;
}

void UMIDIEditorBase::Entry_Initializations()
{
	UE_LOG(LogTemp, Log, TEXT("Entry initializations %s"), *this->GetName())
	OnMetaBuilderReady.Broadcast();
}

void UMIDIEditorBase::SetBuilderHelper(UMetasoundBuilderHelperBase* InBuilderHelper)
{
	BuilderHelper = InBuilderHelper;
}

UMetasoundBuilderHelperBase* UMIDIEditorBase::GetBuilderHelper()
{

	return GetActiveSessionData()->MetasoundBuilderHelper;
}

void UMIDIEditorBase::SetGeneratorHandle(UMetasoundGeneratorHandle* InGeneratorHandle)
{
	GeneratorHandle = InGeneratorHandle;
}

UMetasoundGeneratorHandle* UMIDIEditorBase::GetGeneratorHandle()
{
	if (SceneManager != this) return SceneManager->GetGeneratorHandle();
	
	return GeneratorHandle;
}




void UMIDIEditorBase::SetWorldContextObject(UObject* InWorldContextObject)
{
	for (const auto& graph : InternalGraphs)
	{
		graph->WorldContextObject = InWorldContextObject;
	}
	WorldContextObject = InWorldContextObject;
}

void UMIDIEditorBase::SetSceneManager(TScriptInterface<IBK_MusicSceneManagerInterface> InSceneManager)
{
	if (InSceneManager) {
	SceneManager = InSceneManager;
	HarmonixMidiFile = SceneManager->GetActiveSessionData()->TimeStampedMidis[0].MidiFile;
	//DataPointer = InSceneManager->GetActiveSessionData();
	InitFromDataHarmonix();
	}
	else {
		SceneManager = this;
		InitFromDataHarmonix();
	}
	
	
	UE_LOG(BKMidiLogs,Log, TEXT("Set Scene Manager, Scene Manager is %s"), *SceneManager.GetObject()->GetFullName())
}

const UObject* UMIDIEditorBase::GetCurrentSceneManager()
{
	return SceneManager.GetObjectRef();
}

void UMIDIEditorBase::SetPerformanceComponent(UAudioComponent* InPerformanceComponent)
{
	//for (auto& graph : InternalGraphs) graph->PerformanceComponent = InPerformanceComponent;

	//PerformanceComponent = InPerformanceComponent;
}

void UMIDIEditorBase::AddHorizontalOffset(float deltaX)
{
	//UE_LOG(LogTemp, Warning, TEXT("The float value is: %f"), deltaX);
	for (TSharedRef<SPianoRollGraph> graph : InternalGraphs)
	{
		graph->AddHorizontalX(deltaX);
	}
}

void UMIDIEditorBase::UpdateTemporalZoom(float newTemporalZoom, const FVector2D& WidgetSpaceZoomOrigin)
{
	for (TSharedRef<SPianoRollGraph> graph : InternalGraphs)
	{
		//const FVector2D PointToMaintainGraphSpace = WidgetSpaceZoomOrigin / graph->horizontalZoom - graph->positionOffset.X;
		graph->hZoomTarget = newTemporalZoom;
		//graph->positionOffset.X = (PointToMaintainGraphSpace.X - WidgetSpaceZoomOrigin.X / newTemporalZoom);
	}
}

void UMIDIEditorBase::ResizePanel(int panelID, int DeltaSize)
{
	InternalGraphs[panelID]->verticalHeight += DeltaSize;
}

FLinearColor UMIDIEditorBase::TrackColorPickerClicked(int indexInTrackOptionsArray)
{
	//SColorPicker-
	
	return GetActiveSessionData()->TrackDisplayOptionsMap[indexInTrackOptionsArray].trackColor;
}



void UMIDIEditorBase::InitFromDataHarmonix()
{


	InternalGraphs.Empty();
	UDAWSequencerData* SessionPtr = nullptr;

	if(MidiEditorCache) MidiEditorCache->GetOrCreateSessionDataForMidiFile(HarmonixMidiFile.Get(), SessionPtr);

	PreviewCache = TSharedPtr<UDAWSequencerData>(SessionPtr);

	//TSharedPtr<SVerticalBox> trackMenuArea;
	MidiEditorSharedPtr = TSharedPtr<ITimeSyncedPanel>(this);
	//int BPM = 60;
	//float pixelsPerBeat = (float)(1000 * mainBPM) / 60;

	panelMainScrollArea->ClearChildren();

	TSharedPtr<SPianoRollGraph> PianoRollGraph;
	PianoRollGraph = SNew(SPianoRollGraph)
		.Clipping(EWidgetClipping::ClipToBounds)
		.gridBrush(GridBrush)
		.gridColor(gridLineColor)
		.accidentalGridColor(accidentalGridLineColor)
		.cNoteColor(cNoteColor)
		.noteColor(noteColor)
		.pixelsPerBeat(100)
		.SessionData(PreviewCache);


	SAssignNew(tracksVerticalBox, SVerticalBox);

	panelMainScrollArea->AddSlot()
		.FillSize(1.0)
		[
			SNew(SHorizontalBox)
				//+ SHorizontalBox::Slot()
				//.FillWidth(0.1)
				//[
				//	SAssignNew(tracksVerticalBox, SVerticalBox)
				//]
				+ SHorizontalBox::Slot()
				.FillWidth(0.9)
				[
					PianoRollGraph.ToSharedRef()
				]
		];

	int numTracks = 0;
	int numTracksRaw = 0;
	//GetActiveSessionData()->TimeStampedMidis.Empty();


	PianoRollGraph->HarmonixMidiFile =  HarmonixMidiFile;
	PianoRollGraph->KeyMappings = KeyMapDataAsset;
	PianoRollGraph->selfSharedPtr = PianoRollGraph;
	PianoRollGraph->parentMidiEditor = MidiEditorSharedPtr;
	//PianoRollGraph->InitFromMidiFile(HarmonixMidiFile.Get());


	//PianoRollGraph->InitFromLinkedMidiData();
	InternalGraphs.Add(PianoRollGraph.ToSharedRef());
	donePopulatingDelegate.Broadcast();
	//PopulateTracksFromData();


}

void UMIDIEditorBase::UpdateMidiFile()
{
	auto CurrentData = GetActiveSessionData();
	//CurrentData->TimeStampedMidis.Empty();
	InitFromDataHarmonix();
}

void UMIDIEditorBase::FindSceneManagerPieCounterpart()
{
	if(SceneManager && SceneManager != this) SceneManager = EditorUtilities::GetSimWorldCounterpartActor((AActor*&) SceneManager);
}

void UMIDIEditorBase::FindSceneManagerEditorCounterpart()
{
	if (SceneManager && SceneManager != this) SceneManager = EditorUtilities::GetEditorWorldCounterpartActor((AActor*&)SceneManager);
}

void UMIDIEditorBase::UpdateDataAsset()
{
	if (GEditor && GEditor->CanTransact() && ensure(!GIsTransacting))
		GEditor->BeginTransaction(TEXT(""), INVTEXT("Update DAW Sequence"), nullptr);
	const auto& MidiName = FName(HarmonixMidiFile->GetName());
	
	//if(MidiEditorCache->CachedSessions.Conatains(MidiName))
	
	UDAWSequencerData* CacheForCurrentMidi = GetActiveSessionData();
	if (CacheForCurrentMidi)
	{
		//For now until we stop hacking it and allow more than one midi file we clear the array
		CacheForCurrentMidi->TimeStampedMidis.Empty();

		//CacheForCurrentMidi->TimeStampedMidis.Add(FTimeStamppedMidiContainer(FMusicTimestamp{ 0,0 }, HarmonixMidiFile.Get(), true, tracksDisplayOptions));
		CacheForCurrentMidi->CalculateSequenceDuration();
		CacheForCurrentMidi->MarkPackageDirty();

		if (TransportWidget) TransportWidget->SetTransportDuration(CacheForCurrentMidi->SequenceDuration * .001f);

		//MidiEditorCache->CachedSessions.Add(MidiName, CacheForCurrentMidi);
		//MidiEditorCache->MarkPackageDirty();
		if (GEditor) GEditor->EndTransaction();
	}



}

void UMIDIEditorBase::PopulateTracksFromData()
{

}

bool UMIDIEditorBase::getFollowCursor()
{

	return bFollowCursor;
}

void UMIDIEditorBase::setFollowCursor(bool inFollowCursor)
{
	bFollowCursor = inFollowCursor;
}

void UMIDIEditorBase::SetTransportPlayState(EBKPlayState newPlayState)
{
	PlaybackStateDelegate.Broadcast(newPlayState);
	for (const auto& Graph : InternalGraphs)
	{
		Graph->TransportPlaystate = newPlayState;
	}
}

void UMIDIEditorBase::ReceiveTransportCommand(EBKTransportCommands newCommand)
{
	// if we have a scene manager we just pass the command 
	if (SceneManager && SceneManager != this)
	{
		SceneManager->SendTransportCommand(newCommand);
		if(TransportWidget)
		{
			TransportWidget->TransportPlayState = SceneManager->GetCurrentPlaybackState();
		}
		
		
		return;
	}

	SendTransportCommand(newCommand);

}

void UMIDIEditorBase::UpdatePatchInTrack(int TrackID, const TScriptInterface<IMetaSoundDocumentInterface> MidiPatchClass)
{
	
	TRANSACT("Update Patch Data")

		
	GetActiveSessionData()->TrackDisplayOptionsMap[TrackID].MidiPatchClass = MidiPatchClass;
	GetActiveSessionData()->MarkPackageDirty();
	if (GEditor) GEditor->EndTransaction();
}

void UMIDIEditorBase::UpdateVolumeInTrack(int TrackID, float newGain)

{
	if (GEditor && GEditor->CanTransact() && ensure(!GIsTransacting))
		GEditor->BeginTransaction(TEXT(""), INVTEXT("Update DAW Sequence"), nullptr);
	
	
	GetActiveSessionData()->TrackDisplayOptionsMap[TrackID].TrackVolume = newGain;
	GetActiveSessionData()->MarkPackageDirty();

	if (GEditor) GEditor->EndTransaction();
}

FTrackDisplayOptions& UMIDIEditorBase::GetTrackOptionsRef(int TrackID)
{
	return GetActiveSessionData()->GetTrackOptionsRef(TrackID);
}

void UMIDIEditorBase::SetGridQuantization(EMusicTimeSpanOffsetUnits newQuantization)
{
	GridQuantizationUnit = newQuantization;
	for (auto& graph : InternalGraphs)
	{
		graph->QuantizationGridUnit = newQuantization;
		graph->RecalcGrid();
	}

	GridChangedDelegate.Broadcast(newQuantization);
}



void UMIDIEditorBase::ToggleTrackVisibility(int trackID, bool inIsVisible)
{
	GetActiveSessionData()->TrackDisplayOptionsMap[trackID].isVisible = inIsVisible;
}

void UMIDIEditorBase::SelectTrack(int trackID)
{
	if (CurrentSelectionIndex == trackID)
	{
		GetActiveSessionData()->TrackDisplayOptionsMap[trackID].isSelected = false;
	}

	if(CurrentSelectionIndex != -1)	GetActiveSessionData()->TrackDisplayOptionsMap[CurrentSelectionIndex].isSelected = false;
	GetActiveSessionData()->TrackDisplayOptionsMap[trackID].isSelected = true;

	OnTrackSelectedDelegate.Broadcast(GetActiveSessionData()->TrackDisplayOptionsMap[trackID].isSelected, trackID);

	CurrentSelectionIndex = trackID;
	for (auto graph : InternalGraphs)
	{

		if (graph.Get().selectedTrackIndex == trackID)
		{
	
			graph.Get().selectedTrackIndex = -1;
			graph.Get().someTrackIsSelected = false;
		}else {
	
			graph.Get().availableSamplesMap = GetActiveSessionData()->TrackDisplayOptionsMap[trackID].SampleAvailabilityMap;
			graph.Get().someTrackIsSelected = true;
			graph.Get().selectedTrackIndex = trackID;
			graph.Get().UpdateSlotsZOrder();
			
		}

	}
	
}

FTrackDisplayOptions& UMIDIEditorBase::GetTracksDisplayOptions(int ID)
{
	return GetActiveSessionData()->GetTracksDisplayOptions(ID); // TODO: insert return statement here
}


void UMIDIEditorBase::SetCurrentPosition(float newCursorPosition)
{
	SetCurrentTimelinePosition(newCursorPosition);
	//SeekEventDelegate.Broadcast(newCursorPosition);
}


TEnumAsByte<EPianoRollEditorMouseMode> UMIDIEditorBase::getCurrentInputMode()
{
	return inputMode;
}

void UMIDIEditorBase::SetInputMode(EPianoRollEditorMouseMode newMode)
{
	for (auto& graph : InternalGraphs) graph->SetInputMode(newMode);

	inputMode = newMode;
}



FReply UMIDIEditorBase::TrackVisibilityClicked(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//UE_LOG(LogTemp, Warning, TEXT("The float value is: %d"), trackIndex);
	
	return FReply::Handled();
}

TMap<int32, UFusionPatch*> UMIDIEditorBase::GetTracksMap()
{
	TMap<int32, UFusionPatch*> newTrackMap;
	for (const auto& [index, track] : GetActiveSessionData()->TrackDisplayOptionsMap)
	{
		newTrackMap.Add(TTuple<int32, UFusionPatch*>(track.TrackIndexInParentMidi, track.fusionPatch.Get()));
	}
	
	return newTrackMap;
}

void UMIDIEditorBase::InitAudioBlock()
{
	if (SceneManager == this)
	{
		UE_LOG(BKMidiLogs, Verbose, TEXT("Initializing Audio Block with no Scene Manager, Editor Preview Only"))
	}
	else {
		UE_LOG(BKMidiLogs, Verbose, TEXT("Initializing Audio Block for Scene Manager Actor: %s"), *SceneManager.GetObject()->GetName())
	}
}

//Too risky...
const TMap<int, FTrackDisplayOptions>& UMIDIEditorBase::GetTrackDisplayOptions()
{
	
	//GetActiveSessionData()->TrackDisplayOptionsMap
	return GetActiveSessionData()->TrackDisplayOptionsMap;
	
	
}

void UMIDIEditorBase::ExecuteAudioParamOnPerformanceComponent(FString InName, float inValue)
{
	if (IsValid(GetAudioComponent())) {
		GetAudioComponent()->SetParameter(FAudioParameter(FName(InName), inValue));
		
	}
}



void UMIDIEditorBase::UpdateElementInDisplayOptions(int ElementID, UPARAM(ref)FTrackDisplayOptions& InTrackOptions)
{
	TRANSACT("Update Track Data")
	
	//GetActiveSessionData()->TrackDisplayOptionsMap[ElementID] = InTrackOptions;
	GetActiveSessionData()->MarkPackageDirty();
	if (GEditor) GEditor->EndTransaction();
}

void UMIDITrackCache::GetOrCreateSessionDataForMidiFile(UMidiFile* MidiFile, UDAWSequencerData*& OutSessionData)
{
	const FString MidiName = MidiFile->GetName();
	
	if (CachedSessions.Contains(MidiName))
	{
		OutSessionData = CachedSessions[MidiName];
	}
	else {
		OutSessionData = NewObject<UDAWSequencerData>(this);
		OutSessionData->TimeStampedMidis.Add(FTimeStamppedMidiContainer(FMusicTimestamp{ 0,0 }, MidiFile, true, TArray<FTrackDisplayOptions>()));
		OutSessionData->PopulateFromMidiFile(MidiFile);
		CachedSessions.Add(MidiName, OutSessionData);
		MarkPackageDirty();
	}
}
