// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Events.h"
#include "Widgets/SCompoundWidget.h"
#include "M2SoundGraphData.h"
#include "UndawMusicDrawingStatics.h"
#include "SlateFwd.h"
#include "Components/Widget.h"


/**
implementations for panning and zooming

*/

namespace UnDAW
{
	const TRange<int> MidiNoteRange{ 0, 127 };
}



class SMidiEditorPanelBase: public SCompoundWidget
{
public:

	float HorizontalZoom = 1.0f;
	float VerticalZoom = 1.0f;
	float HorizontalOffset = 0.0f;
	float VerticalOffset = 0.0f;
	bool bIsPanActive = false;
	bool bLockVerticalPan = false;
	UnDAW::FGridPointMap GridPoints;
	UnDAW::EGridPointType GridPointType = UnDAW::EGridPointType::Bar;
	UnDAW::EMusicTimeLinePaintMode PaintMode = UnDAW::EMusicTimeLinePaintMode::Music;
	UnDAW::ETimeDisplayMode TimeDisplayMode = UnDAW::ETimeDisplayMode::TimeLinear;

	UDAWSequencerData* SequenceData = nullptr;

	TRange<int> ContentRange;

	TOptional<EMouseCursor::Type> GetCursor() const override
	{

		if (bIsPanActive)
		{
			return EMouseCursor::GrabHand;
		}

		return TOptional<EMouseCursor::Type>();
	}


	FReply OnMousePan(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			HorizontalOffset += MouseEvent.GetCursorDelta().X;
			if(!bLockVerticalPan) VerticalOffset += MouseEvent.GetCursorDelta().Y;
			bIsPanActive = true;
			return FReply::Handled();
		}
		bIsPanActive = false;
		return FReply::Unhandled();
	}

	FReply OnZoom(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		
		bool bIsCtrlPressed = MouseEvent.IsControlDown();
		bool bIsShiftPressed = MouseEvent.IsShiftDown();
		
		
		if (MouseEvent.GetWheelDelta() > 0)
		{			
			if (bIsShiftPressed)
			{
				VerticalZoom += 0.1f;
			}
			else if (bIsCtrlPressed)
			{
				HorizontalZoom *= 1.1f;
			}
			else {
				OnVerticalScroll(MouseEvent.GetWheelDelta());
			}
		}
		else
		{
			if (bIsShiftPressed)
			{
				VerticalZoom -= 0.1f;
			}
			else if (bIsCtrlPressed)
			{
				HorizontalZoom *= 0.9f;
			}
			else {
				OnVerticalScroll(MouseEvent.GetWheelDelta());
			}
		}

		return FReply::Handled();
	}


	const float TickToPixel(const float Tick) const
	{
		return SequenceData->HarmonixMidiFile->GetSongMaps()->TickToMs(Tick - HorizontalOffset) * HorizontalZoom;
	}

	virtual void OnVerticalScroll(float ScrollAmount) {};

	FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return OnMousePan(MyGeometry, MouseEvent);
	}

	FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{

		return OnZoom(MyGeometry, MouseEvent);
	}

	virtual void CacheDesiredSize(float InLayoutScaleMultiplier) override {
		RecalculateGrid();
	}
	
	void RecalculateGrid()
	{
		GridPoints.Empty();
		const auto& SongsMap = SequenceData->HarmonixMidiFile->GetSongMaps();
		const auto& BarMap = SongsMap->GetBarMap();
		const auto& BeatMap = SongsMap->GetBeatMap();
		//const auto& SubdivisionMap = SongsMap->GetSubdivisionMap();

		const float FirstTickOfFirstBar = SongsMap->GetBarMap().BarBeatTickIncludingCountInToTick(1, 1, 0);
		const float LastTickOfLastBar = SequenceData->HarmonixMidiFile->GetLastEventTick();

		using namespace UnDAW;


		int32 BarCount = 1;
		float BarTick = 0;

		while (BarTick <= LastTickOfLastBar)
		{
			//VisibleBars.Add(MidiSongMap->CalculateMidiTick(MidiSongMap->GetBarMap().TickToMusicTimestamp(BarTick), TimeSpanToSubDiv(EMusicTimeSpanOffsetUnits::Bars)));
			GridPoints.Add(BarTick, { EGridPointType::Bar, BarCount++, 1, 1 });
			BarTick += SongsMap->SubdivisionToMidiTicks(EMidiClockSubdivisionQuantization::Bar, BarTick);
		}

		//for (const auto& Beat : BeatMap)
		//{
		//	FMusicalGridPoint Point;
		//	Point.Type = EGridPointType::Beat;
		//	Point.Bar = Beat.Value.Bar;
		//	Point.Beat = Beat.Value.Beat;
		//	GridPoints.Add(Beat.Key, Point);
		//}

		//for (const auto& Subdivision : SubdivisionMap)
		//{
		//	FMusicalGridPoint Point;
		//	Point.Type = EGridPointType::Subdivision;
		//	Point.Bar = Subdivision.Value.Bar;
		//	Point.Beat = Subdivision.Value.Beat;
		//	Point.Subdivision = Subdivision.Value.Subdivision;
		//	GridPoints.Add(Subdivision.Key, Point);
		//}
	}

	int32 PaintTimelineMarks(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;

};

/**
 * 
 */
class BKMUSICWIDGETS_API SMidiClipEditor : public SMidiEditorPanelBase
{
public:
	SLATE_BEGIN_ARGS(SMidiClipEditor)
	{}
	SLATE_END_ARGS()

	FText TrackMetaDataName = FText::GetEmpty();
	int32 TrackIndex = INDEX_NONE;
	FLinearColor TrackColor = FLinearColor::White;
	
	FLinkedNotesClip* Clip = nullptr;

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, UDAWSequencerData* InSequence);

	void OnClipsFocused(TArray< TTuple<FDawSequencerTrack*, FLinkedNotesClip*> > Clips);

	int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;



	void ZoomToContent()
	{
		const auto& CachedGeometry = GetCachedGeometry();
		const auto& SongsMap = SequenceData->HarmonixMidiFile->GetSongMaps();

		const float Width = SongsMap->TickToMs(Clip->EndTick) - SongsMap->TickToMs(Clip->StartTick);

		HorizontalZoom = CachedGeometry.Size.X / Width;
		HorizontalOffset = -SongsMap->TickToMs(Clip->StartTick);

		TRange<int8> ClipNoteRange { Clip->MinNote, Clip->MaxNote };

		VerticalZoom = CachedGeometry.Size.Y / (ClipNoteRange.Size<int8>() * UnDAW::MidiNoteRange.Size<int8>());

		const float Height = (CachedGeometry.Size.Y / 127) / VerticalZoom;
		VerticalOffset = -(127 - Clip->MaxNote) * Height;

	}

};
