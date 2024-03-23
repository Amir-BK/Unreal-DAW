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

struct FEventsWithIndex
{
	FMidiEvent event;
	int32 eventIndex;
};

void UMIDIEditorBase::SetCurrentTimelinePosition(float inPosition)
{
	
	if (currentTimelineCursorPosition == inPosition) return;
	
	if (SceneManager)
	{
		SceneManager->SendSeekCommand(inPosition);

	}
	else {
		//SeekEventDelegate.Broadcast(inPosition);
		currentTimelineCursorPosition = inPosition;
	}

	if (TransportWidgetInstance)
	{
		TransportWidgetInstance->SetTransportSeek(inPosition);
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

	
	if (TransportWidgetInstance)
	{
		TransportWidgetInstance->TransportCalled.AddUniqueDynamic(this, &UMIDIEditorBase::ReceiveTransportCommand);
		TransportWidgetInstance->TransportSeekCommand.AddUniqueDynamic(this, &UMIDIEditorBase::SetCurrentPosition);
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
	if (SceneManager != this) return SceneManager->GetAudioComponent();

		return UGameplayStatics::CreateSound2D(WorldContextObject, nullptr, 1.0f, 1.0f, 0.0f);
}

UDAWSequencerData* UMIDIEditorBase::GetActiveSessionData()
{
	if (SceneManager == this)
	{
		UDAWSequencerData** MapRef = nullptr;
		if (HarmonixMidiFile != nullptr) {
			
			const auto& MidiName = FName(HarmonixMidiFile->GetName());
			MapRef = MidiEditorCache->CachedSessions.Find(MidiName);
		}
	
		UDAWSequencerData* CacheForCurrentMidi;
		if (!MapRef)
		{
			CacheForCurrentMidi = NewObject<UDAWSequencerData>(MidiEditorCache);
		}
		else {
			CacheForCurrentMidi = *MapRef;
		}
		return CacheForCurrentMidi;
	}
	return SceneManager->GetActiveSessionData();
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

	InitFromDataHarmonix();
	}
	else {
		SceneManager = this;
	}
	
	
	UE_LOG(LogTemp,Log, TEXT("Set Scene Manager, Scene Manager is %s"), *SceneManager.GetObject()->GetName())
}

void UMIDIEditorBase::SetPerformanceComponent(UAudioComponent* InPerformanceComponent)
{
	for (auto& graph : InternalGraphs) graph->PerformanceComponent = InPerformanceComponent;

	PerformanceComponent = InPerformanceComponent;
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
	return tracksDisplayOptions[indexInTrackOptionsArray].trackColor;
}



void UMIDIEditorBase::InitFromDataHarmonix()
{



	tracksDisplayOptions.Empty();
	InternalGraphs.Empty();

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
		.pixelsPerBeat(100);


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


	if (HarmonixMidiFile == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("No midi file! This is strange!"))
			return;
	}
	for (auto& track : HarmonixMidiFile->GetTracks())
	{
		//if track has no events we can continue, but this never happens, it might not have note events but it has events.
		int numTracksInternal = numTracksRaw++;
		if(track.GetEvents().IsEmpty()) continue;
		
		TMap<int, TArray<FLinkedMidiEvents*>> channelsMap;
		
		

		TArray<FLinkedMidiEvents*> linkedNotes;
		//TMap<int32, FMidiEvent> unlinkedNotes;
		TMap<int32, FEventsWithIndex> unlinkedNotesIndexed;
		
		//track.GetEvent(32)

		// sort events, right now only notes 
		for (int32 index = 0; const auto& MidiEvent : track.GetEvents())
		{
			switch(MidiEvent.GetMsg().Type) {
			case FMidiMsg::EType::Std:
				if(MidiEvent.GetMsg().IsNoteOn())
				{
					//unlinkedNotes.Add(MidiEvent.GetMsg().GetStdData1(), MidiEvent);
					unlinkedNotesIndexed.Add(MidiEvent.GetMsg().GetStdData1(), FEventsWithIndex{ MidiEvent, index });
				};

				if(MidiEvent.GetMsg().IsNoteOff())
				{
					if(unlinkedNotesIndexed.Contains(MidiEvent.GetMsg().GetStdData1()))
					{
						const int midiChannel = MidiEvent.GetMsg().GetStdChannel();
						//MidiEvent.GetMsg().
						//unlinkedNotesIndexed
						if (midiChannel == unlinkedNotesIndexed[MidiEvent.GetMsg().GetStdData1()].event.GetMsg().GetStdChannel())
						{
							
							FLinkedMidiEvents* foundPair = new FLinkedMidiEvents(unlinkedNotesIndexed[MidiEvent.GetMsg().GetStdData1()].event, MidiEvent,
								unlinkedNotesIndexed[MidiEvent.GetMsg().GetStdData1()].eventIndex, index);
							linkedNotes.Add(foundPair);
							// sort the tracks into channels
							if (channelsMap.Contains(midiChannel))
							{
								channelsMap[midiChannel].Add(foundPair);
							}
							else {
								channelsMap.Add(TTuple<int, TArray<FLinkedMidiEvents*>>(midiChannel, TArray<FLinkedMidiEvents*>()));
								channelsMap[midiChannel].Add(foundPair);
							}
						
						}

					}
				};

				break;
			case FMidiMsg::EType::Tempo:
				UE_LOG(BKMidiLogs, Verbose, TEXT("We receive a tempo event! data1 %d data2 %d"), MidiEvent.GetMsg().Data1, MidiEvent.GetMsg().Data2)
					//MidiEvent.GetMsg().Data1
					TempoEvents.Add(MidiEvent);
				break;
			case FMidiMsg::EType::TimeSig:
				UE_LOG(BKMidiLogs, Verbose, TEXT("We receive a time signature event!"))
					TimeSignatureEvents.Add(MidiEvent);
				break;
			case FMidiMsg::EType::Text:
				UE_LOG(BKMidiLogs, Verbose, TEXT("We receive a text event??? %s"), *MidiEvent.GetMsg().ToString(MidiEvent.GetMsg()))
				break;
			case FMidiMsg::EType::Runtime:
				UE_LOG(BKMidiLogs, Verbose, TEXT("We receive a runtime event???"))
				break;
			}

			++index;
		
		}
		// if we couldn't find any linked notes this track is a control track, contains no notes.
		
		if(channelsMap.IsEmpty()) continue;

		UE_LOG(BKMidiLogs, Log, TEXT("Num Channel Buckets: %d"), channelsMap.Num())
			
			bool hasCacheFile = IsValid(MidiEditorCache.Get()) && MidiEditorCache->CachedSessions.Contains(HarmonixMidiFile->GetFName());
		

		//if (track->contentEvents.IsEmpty()) continue;
			for (auto& [channel, notes] : channelsMap)
			{
				FColorPickerArgs PickerArgs;
				//this is the init code for the display options, should be moved to it's own functions
				bool hasDataForTrack = false;

				FTrackDisplayOptions newTrackDisplayOptions;
				if (hasCacheFile)
				{	
					const auto TrackCache = *MidiEditorCache->CachedSessions.Find(HarmonixMidiFile->GetFName());
					
					if (TrackCache && TrackCache->TimeStampedMidis[0].TracksMappings.IsValidIndex(numTracks))
					{
						newTrackDisplayOptions = TrackCache->TimeStampedMidis[0].TracksMappings[numTracks];
						tracksDisplayOptions.Add(TrackCache->TimeStampedMidis[0].TracksMappings[numTracks]);
						hasDataForTrack = true;
					}
					
				}
				
				if(!hasDataForTrack){
					newTrackDisplayOptions = FTrackDisplayOptions();
					newTrackDisplayOptions.fusionPatch = DefaultFusionPatch;
					newTrackDisplayOptions.TrackIndexInParentMidi = numTracksInternal;
					newTrackDisplayOptions.ChannelIndexInParentMidi = channelsMap.Num() == 1 ? 0 : channel;
					newTrackDisplayOptions.trackColor = FLinearColor::MakeRandomSeededColor((numTracksInternal + 1) * (channel + 1));
					newTrackDisplayOptions.trackName = *track.GetName();
					tracksDisplayOptions.Add(newTrackDisplayOptions);
				}


				UE_LOG(LogTemp, Log, TEXT("%s"), HarmonixMidiFile->GetFName());

				if (!notes.IsEmpty())
				{
					for (const auto& foundPair : notes)
					{
						PianoRollGraph->AddNote(*foundPair, numTracks, numTracksInternal);
					}
				}
				numTracks++;
			
			
			}

	};

	tracksVerticalBox->AddSlot()
		[
			SNew(SBorder)
				.Padding(5.0f)
					[ 
						SNew(SMidiEditorBaseSettingsWidget)
						.parentMidiEditor(MidiEditorSharedPtr)
					]
		];


	InternalGraphs.Add(PianoRollGraph.ToSharedRef());

	
	PianoRollGraph->parentMidiEditor = MidiEditorSharedPtr;

	donePopulatingDelegate.Broadcast();
	UpdateDataAsset();

	PianoRollGraph->NeedsRinitDelegate.AddDynamic(this, &UMIDIEditorBase::InitFromDataHarmonix);
}

void UMIDIEditorBase::UpdateDataAsset()
{
//	if (GEditor && GEditor->CanTransact() && ensure(!GIsTransacting))
	//	GEditor->BeginTransaction(TEXT(""), INVTEXT("Update DAW Sequence"), nullptr);
	const auto& MidiName = FName(HarmonixMidiFile->GetName());
	FTimeStamppedMidiContainer(FMusicTimestamp{ 0,0 }, HarmonixMidiFile.Get(), true, tracksDisplayOptions);
	//if(MidiEditorCache->CachedSessions.Conatains(MidiName))
	
	UDAWSequencerData* CacheForCurrentMidi;
	
	if (SceneManager == this) {
		UDAWSequencerData** MapRef = MidiEditorCache->CachedSessions.Find(MidiName);
		
		if (!MapRef)
		{
			CacheForCurrentMidi = NewObject<UDAWSequencerData>(MidiEditorCache, MidiName);
		}
		else {
			CacheForCurrentMidi = *MapRef;
		}
	}
	else {
		CacheForCurrentMidi = GetActiveSessionData();
	}


	//For now until we stop hacking it and allow more than one midi file we clear the array
	CacheForCurrentMidi->TimeStampedMidis.Empty();

	CacheForCurrentMidi->TimeStampedMidis.Add(FTimeStamppedMidiContainer(FMusicTimestamp{ 0,0 }, HarmonixMidiFile.Get(), true, tracksDisplayOptions));
	CacheForCurrentMidi->CalculateSequenceDuration();

	if (TransportWidgetInstance) TransportWidgetInstance->SetTransportDuration(CacheForCurrentMidi->SequenceDuration * .001f);

	MidiEditorCache->CachedSessions.Add(MidiName, CacheForCurrentMidi);
	MidiEditorCache->MarkPackageDirty();
//	if (GEditor) GEditor->EndTransaction();

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
	if (SceneManager)
	{
		SceneManager->SendTransportCommand(newCommand);
		if(TransportWidgetInstance)
		{
			TransportWidgetInstance->TransportPlayState = SceneManager->GetCurrentPlaybackState();
		}
		
		
		return;
	}
	switch (newCommand)
	{
	case Init:
		UpdateDataAsset();
		// create builder
		//UE_LOG(LogTemp, Log, TEXT("Received Init"))
		break;
	case Play:

		//UE_LOG(LogTemp,Log, TEXT("Received Play"))
		break;
	case Pause:
		break;
	case Stop:
		break;
	case Kill:
		break;
	case TransportBackward:
		break;
	case TransportForward:
		break;
	case NextMarker:
		break;
	case PrevMarker:
		break;
	default:
		break;
	}

}

void UMIDIEditorBase::UpdatePatchInTrack(int TrackID, const TScriptInterface<IMetaSoundDocumentInterface> MidiPatchClass)
{
	tracksDisplayOptions[TrackID].MidiPatchClass = MidiPatchClass;
}

void UMIDIEditorBase::UpdateVolumeInTrack(int TrackID, float newGain)

{
	tracksDisplayOptions[TrackID].TrackVolume = newGain;
}

FTrackDisplayOptions& UMIDIEditorBase::GetTrackOptionsRef(int TrackID)
{
	return tracksDisplayOptions[TrackID];
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
	tracksDisplayOptions[trackID].isVisible = inIsVisible;
}

void UMIDIEditorBase::SelectTrack(int trackID)
{
	if (CurrentSelectionIndex == trackID)
	{
		tracksDisplayOptions[trackID].isSelected = false;
	}
	tracksDisplayOptions[CurrentSelectionIndex].isSelected = false;
	tracksDisplayOptions[trackID].isSelected = true;

	OnTrackSelectedDelegate.Broadcast(tracksDisplayOptions[trackID].isSelected, trackID);

	CurrentSelectionIndex = trackID;
	for (auto graph : InternalGraphs)
	{

		if (graph.Get().selectedTrackIndex == trackID)
		{
	
			graph.Get().selectedTrackIndex = -1;
			graph.Get().someTrackIsSelected = false;
		}else {
	
			graph.Get().availableSamplesMap = tracksDisplayOptions[trackID].SampleAvailabilityMap;
			graph.Get().someTrackIsSelected = true;
			graph.Get().selectedTrackIndex = trackID;
			graph.Get().UpdateSlotsZOrder();
			
		}

	}
	
}

FTrackDisplayOptions& UMIDIEditorBase::GetTracksDisplayOptions(int ID)
{
	return tracksDisplayOptions[ID];
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
	for (auto& track : tracksDisplayOptions)
	{
		newTrackMap.Add(TTuple<int32, UFusionPatch*>(track.TrackIndexInParentMidi, track.fusionPatch.Get()));
	}
	
	return newTrackMap;
}

void UMIDIEditorBase::InitAudioBlock()
{
	if (SceneManager == nullptr)
	{
		UE_LOG(BKMidiLogs, Verbose, TEXT("Initializing Audio Block with no Scene Manager, Editor Preview Only"))
	}
	else {
		UE_LOG(BKMidiLogs, Verbose, TEXT("Initializing Audio Block for Scene Manager Actor: %s"), *SceneManager.GetObject()->GetName())
	}
}

TArray<FTrackDisplayOptions>& UMIDIEditorBase::GetTrackDisplayOptions()
{
	return tracksDisplayOptions;
}

void UMIDIEditorBase::ExecuteAudioParamOnPerformanceComponent(FString InName, float inValue)
{
	if (IsValid(PerformanceComponent)) {
		PerformanceComponent->SetParameter(FAudioParameter(FName(InName), inValue));
		
	}
}



void UMIDIEditorBase::UpdateElementInDisplayOptions(int ElementID, UPARAM(ref)FTrackDisplayOptions& InTrackOptions)
{
	tracksDisplayOptions[ElementID] = InTrackOptions;
}



