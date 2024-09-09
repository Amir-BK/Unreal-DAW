// Fill out your copyright notice in the Description page of Project Settings.


#include "Sequencer/MidiClipEditor/SMidiClipEditor.h"
#include "SlateOptMacros.h"
#include "Styling/AppStyle.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMidiClipEditor::Construct(const FArguments& InArgs, UDAWSequencerData* InSequence)
{
	SequenceData = InSequence;
	/*
	ChildSlot
	[
		// Populate the widget
	];
	*/
}
void SMidiClipEditor::OnClipsFocused(TArray<TTuple<FDawSequencerTrack*, FLinkedNotesClip*>> Clips)
{
	if (!Clips.IsEmpty())
	{
		TrackIndex = Clips[0].Key->MetadataIndex;
		TrackMetaDataName = FText::FromString(SequenceData->GetTrackMetadata(TrackIndex).TrackName);
		Clip = Clips[0].Value;
		TrackColor = SequenceData->GetTrackMetadata(TrackIndex).TrackColor;

		ZoomToContent();

	}
}
inline int32 SMidiClipEditor::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	auto ScreenCenterPaintGeometry = AllottedGeometry.ToOffsetPaintGeometry(FVector2D(AllottedGeometry.Size / 2.0f));
	auto OffsetPaintGeometry = AllottedGeometry.ToOffsetPaintGeometry(FVector2D(HorizontalOffset, VerticalOffset));

	//draw notes
	const float Height = (AllottedGeometry.Size.Y / 127) / VerticalZoom;
	if (Clip != nullptr)
	{
		LayerId++;
		for (const auto& Note : Clip->LinkedNotes)
		{
			const float Start = TransformTickToPixel(Note.StartTick + Clip->StartTick);
			const float End = TransformTickToPixel(Note.EndTick + Clip->StartTick);
			const float Width = End - Start;
			const float Y = (127 - Note.Pitch) * Height;

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				OffsetPaintGeometry,
				{ FVector2D(Start, Y), FVector2D(Start + Width, Y) },
				ESlateDrawEffect::None,
				TrackColor
			);
		}

	}

	const auto ToPrint = FText::FromString(FString::Printf(TEXT("Track %d\nVZoom: %f\n HZoom: %f"), TrackIndex, HorizontalZoom, VerticalZoom));
	FSlateDrawElement::MakeText(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), ToPrint, FAppStyle::GetFontStyle("NormalFont"), ESlateDrawEffect::None, FLinearColor::White);

	return LayerId;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION