// Fill out your copyright notice in the Description page of Project Settings.


#include "Sequencer/MidiClipEditor/SMidiClipEditor.h"
#include "SlateOptMacros.h"
#include "UnDAWStyle.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMidiClipEditor::Construct(const FArguments& InArgs, UDAWSequencerData* InSequence)
{
	SMidiEditorPanelBase::Construct(InArgs._ParentArgs, InSequence);
	SequenceData = InSequence;
	MajorTabWidth = 0.0f;
	TimelineHeight = InArgs._TimelineHeight;

	//FSoftClassPath NoteBrushPath = FSoftClassPath(TEXT("/unDAW/Brushes/MidiNoteBrush.MidiNoteBrush"));
	//NoteBrush = NoteBrushPath.TryLoad();
	

	/*
	ChildSlot
	[
		// Populate the widget
	];
	*/

	//populate piano grid colors 
	auto GridColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.1f);
		
	auto AccidentalColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.1f);
	auto CNoteGridColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.1f);

	for (int i = 0; i < 128; i++)
	{
		if (i % 12 == 1 || i % 12 == 3 || i % 12 == 6 || i % 12 == 8 || i % 12 == 10)
		{
			PianoGridColors.Add(AccidentalColor);
		}
		else if (i % 12 == 0)
		{
			PianoGridColors.Add(CNoteGridColor);
		}
		else
		{
			PianoGridColors.Add(GridColor);
		}
	}


}
int32 SMidiClipEditor::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	auto ScreenCenterPaintGeometry = AllottedGeometry.ToOffsetPaintGeometry(FVector2D(AllottedGeometry.Size / 2.0f));
	auto OffsetGeometryChild = AllottedGeometry.MakeChild(AllottedGeometry.GetLocalSize(), FSlateLayoutTransform(1.0f, Position.Get()));

	auto TrackAreaGeometry = AllottedGeometry.MakeChild(
		FVector2f(0, TimelineHeight),
		FVector2f(AllottedGeometry.Size.X, AllottedGeometry.Size.Y - TimelineHeight)
	);

	static const FSlateBrush* NoteBrush = FUndawStyle::Get().GetBrush("MidiNoteBrush");


	//draw the piano grid
	LayerId++;
	for (int i = 0; i < 128; i++)
	{
		const float Y = (127 - i) * RowHeight + Position.Get().Y;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D((AllottedGeometry.Size.X), RowHeight), FSlateLayoutTransform(1.0f, FVector2D(0, Y))),
			NoteBrush,
			ESlateDrawEffect::None,
			PianoGridColors[i]
		);

		//if note is C (i % 12 == 0) write the note name
		if (i % 12 == 0)
		{
			const auto NoteName = FText::FromString(FString::Printf(TEXT("C (%d)"), (i / 12) - 2));
			FSlateDrawElement::MakeText(OutDrawElements, LayerId++, AllottedGeometry.ToOffsetPaintGeometry(FVector2D(0, Y)), NoteName, FAppStyle::GetFontStyle("NormalFont"), ESlateDrawEffect::None, FLinearColor::White);
		}
	}




	if (Clip != nullptr)
	{
		LayerId++;
		for (const auto& Note : Clip->LinkedNotes)
		{
			const float Start = TickToPixel(Note.StartTick);
			if (Start > AllottedGeometry.Size.X - Position.Get().X) continue;
			

			const float End = TickToPixel(Note.EndTick);
			if (End < 0 - Position.Get().X) continue;
			const float Width = End - Start;

			if (Width < 0.1) continue;

			const float Y = (127 - Note.Pitch) * RowHeight;

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				OffsetGeometryChild.ToPaintGeometry(FVector2D(Width, RowHeight), FSlateLayoutTransform(1.0f, FVector2D(Start, Y))),
				NoteBrush,
				ESlateDrawEffect::None,
				TrackColor
			);

			//FSlateDrawElement::MakeBox




		}

		PaintTimeline(Args, AllottedGeometry, MyCullingRect, OutDrawElements, ++LayerId);

	}

	PaintPlayCursor(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId);

	const auto ToPrint = FText::FromString(FString::Printf(TEXT("Track %d\nVZoom: %f\n HZoom: %f\n Offset (%f, %f)"), TrackIndex, Zoom.Get().X, Zoom.Get().Y, Position.Get().X, Position.Get().Y));
	FSlateDrawElement::MakeText(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), ToPrint, FAppStyle::GetFontStyle("NormalFont"), ESlateDrawEffect::None, FLinearColor::White);

	return LayerId;
}



int32 SMidiClipVelocityEditor::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	PaintTimeline(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId);
	
	if (Clip != nullptr)
	{
		auto OffsetGeometryChild = AllottedGeometry.MakeChild(AllottedGeometry.GetLocalSize(), FSlateLayoutTransform(1.0f, Position.Get()));
		LayerId++;

		FMidiSustainPedalEvent CurrentSustainOnEvent;

		for (const auto& SustainEvent : Clip->SustainPedalEvents)
		{
			//we draw a transparent box for the sustain pedal events from event to the next event
			//if is pedal on, we store the event

			if (SustainEvent.bIsSustainPedalDown)
			{
				CurrentSustainOnEvent = SustainEvent;
			}
			else
			{
				const float Start = TickToPixel(CurrentSustainOnEvent.StartTick);
				const float End = TickToPixel(SustainEvent.StartTick);
				const float Width = End - Start;
				const float Y = AllottedGeometry.GetLocalSize().Y;
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId,
					OffsetGeometryChild.ToPaintGeometry(FVector2D(Width, Y), FSlateLayoutTransform(1.0f, FVector2D(Start, 0))),
					FAppStyle::GetBrush("Graph.Panel.SolidBackground"),
					ESlateDrawEffect::None,
					FLinearColor(0.1f, 0.1f, 0.1f, 0.01f)
				);
			}	

		}
		
		LayerId++;
		
		for (const auto& Note : Clip->LinkedNotes)
		{
			const float Start = TickToPixel(Note.StartTick);
			
			const float Y = ((127 - Note.NoteVelocity) / 127.0f) * AllottedGeometry.GetLocalSize().Y;

			//paint a line extending from the bottom of the panel to the Y velocity value

			static const FSlateBrush* DottedKeyBarBrush = FAppStyle::GetBrush("Sequencer.KeyBar.Dotted");
			static const FSlateBrush* DashedKeyBarBrush = FAppStyle::GetBrush("Sequencer.KeyBar.Dashed");
			static const FSlateBrush* SolidKeyBarBrush = FAppStyle::GetBrush("Sequencer.KeyBar.Solid");
			static const FSlateBrush* NoteBrush = FUndawStyle::Get().GetBrush("MidiNoteBrush.Selected");
			const float Rotate = FMath::DegreesToRadians(45.f);


			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				OffsetGeometryChild.ToPaintGeometry(FVector2D(1, AllottedGeometry.GetLocalSize().Y), FSlateLayoutTransform(1.0f, FVector2D(Start, 0))),
				{ FVector2D(0, AllottedGeometry.GetLocalSize().Y), FVector2D(0, Y) },
				ESlateDrawEffect::None,
				TrackColor
			);

			//draw a rotated box at the velocity value using the selected note brush
			FSlateDrawElement::MakeRotatedBox(
				OutDrawElements,
				LayerId,
				OffsetGeometryChild.ToPaintGeometry(FVector2D(10, 10), FSlateLayoutTransform(1.0f, FVector2D(Start - 5, Y))),
				NoteBrush,
				ESlateDrawEffect::None,
				Rotate,
				TOptional<FVector2D>(),
				FSlateDrawElement::RelativeToElement,
				TrackColor
			);


			//paint the velcity value as text
			//FSlateDrawElement::MakeText(OutDrawElements, LayerId++, OffsetGeometryChild.ToOffsetPaintGeometry(FVector2D(Start, Y - 10)), FText::FromString(FString::Printf(TEXT("%d"), Note.NoteVelocity)), FAppStyle::GetFontStyle("NormalFont"), ESlateDrawEffect::None, FLinearColor::White);

		}

	

		PaintPlayCursor(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId);

		const auto ToPrint = FText::FromString(FString::Printf(TEXT("Track %d\nVZoom: %f\n HZoom: %f\n Offset (%f, %f)\n Bar, Beat: %d, %f"), 
			TrackIndex, Zoom.Get().X, Zoom.Get().Y, Position.Get().X, Position.Get().Y, PlayCursor.Get().Bar, PlayCursor.Get().Beat));
		FSlateDrawElement::MakeText(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), ToPrint, FAppStyle::GetFontStyle("NormalFont"), ESlateDrawEffect::None, FLinearColor::White);

	}

	
	return LayerId;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION