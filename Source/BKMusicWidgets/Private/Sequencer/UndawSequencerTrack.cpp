#include "Sequencer/UndawSequencerTrack.h"
#include "SlateFwd.h"
#include "Components/Widget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"




void SDAwSequencerTrackControlsArea::Construct(const FArguments& InArgs)
{
	DesiredWidth = InArgs._DesiredWidth;

	ChildSlot
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(ControlsBox, SBox)

				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SSpacer)
						.Size(FVector2D(5, 1))
				]

		];

}

TOptional<EMouseCursor::Type> SDAwSequencerTrackControlsArea::GetCursor() const
{
	//if hovering over the rightmost 5 pixels, show the resize cursor
	if (bIsHoveringOverResizeArea)
	{
		return EMouseCursor::ResizeLeftRight;
	}
	else
	{
		return EMouseCursor::Default;
	}

}

FReply SDAwSequencerTrackControlsArea::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//is hovering over the sspacer are? 5 rightmost pixels)

	const bool bIsLeftMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);

	if (bIsLeftMouseButtonDown)
	{
		if (bIsHoveringOverResizeArea)
		{
			bIsResizing = true;
			return FReply::Handled().CaptureMouse(AsShared());
		}
	}



	return FReply::Unhandled();
}

FReply SDAwSequencerTrackControlsArea::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsLeftMouseButtonAffecting = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;

	if (bIsLeftMouseButtonAffecting)
	{
		if (bIsResizing)
		{
			bIsResizing = false;
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Unhandled();
}

FReply SDAwSequencerTrackControlsArea::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{

	////resize the slot
	const FVector2D LocalMousePosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	////UE_LOG(LogTemp, Warning, TEXT("Mouse position %s, target val %f"), *LocalMousePosition.ToString(), MyGeometry.Size.X - 5);



	if (LocalMousePosition.X > MyGeometry.Size.X - 5)
	{

		bIsHoveringOverResizeArea = true;


	}
	else {
		bIsHoveringOverResizeArea = false;
	}

	if (bIsResizing)
	{
		OnVerticalMajorSlotResized.ExecuteIfBound(LocalMousePosition.X);
	}

	return FReply::Unhandled();
}


void SDawSequencerTrackRoot::Construct(const FArguments& InArgs, UDAWSequencerData* InSequenceToEdit, int32 TrackId)
{
	ChildSlot
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(ControlsArea, SDAwSequencerTrackControlsArea)
				]
				+ SHorizontalBox::Slot()
				[
					SAssignNew(LaneBox, SBox)
						.MinDesiredHeight(150)
						.Content()
						[
							SAssignNew(Lane, SDawSequencerTrackLane, InSequenceToEdit, TrackId)
								.Clipping(EWidgetClipping::ClipToBounds)
								.Zoom(InArgs._Zoom)
								.Position(InArgs._Position)

						]


				]
		];
}



void SDawSequencerTrackLane::Construct(const FArguments& InArgs, UDAWSequencerData* InSequenceToEdit, int32 InTrackId)
{
	SequenceData = InSequenceToEdit;
	TrackId = InTrackId;
	Position = InArgs._Position;
	Zoom = InArgs._Zoom;

	//PopulateSections();

	for (auto& Clip : SequenceData->Tracks[TrackId].LinkedNotesClips)
	{
		TSharedPtr<SDawSequencerTrackMidiSection> Section;


		SAssignNew(Section, SDawSequencerTrackMidiSection, &Clip, &SequenceData->Tracks[TrackId])
			.TrackColor(TAttribute<FLinearColor>::CreateLambda([this]() {return SequenceData->GetTrackMetadata(TrackId).TrackColor; }))
			.Position(InArgs._Position)
			.Zoom(InArgs._Zoom);

		Section->AssignParentWidget(SharedThis(this));
		Sections.Add(Section);
	}

}

TOptional<EMouseCursor::Type> SDawSequencerTrackLane::GetCursor() const {
	if (bIsHoveringOverSectionResizeArea)
	{
		return EMouseCursor::ResizeLeftRight;
	}
	else if (bIsHoveringOverSectionDragArea)
	{
		if (Sections[HoveringOverSectionIndex]->bIsSelected)
			return EMouseCursor::CardinalCross;
		else
			return EMouseCursor::Crosshairs;
	}
	else {
		return TOptional<EMouseCursor::Type>();
	}

}

int32 SDawSequencerTrackLane::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{

	//print zoom values at top right corner
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(AllottedGeometry.Size.X - 300, 0), FSlateLayoutTransform(1.0, FVector2D(100, 20))),
		FText::FromString(FString::Printf(TEXT("Position: %s\nZoom: %s"), *Position.Get().ToString(), *Zoom.Get().ToString())),
		FAppStyle::GetFontStyle("NormalFont"),
		ESlateDrawEffect::None,
		FLinearColor::White
	);

	for (const auto& Section : Sections)
	{
		//auto LayoutTransform = FSlateLayoutTransform(FVector2D(0, 0));
		const float ZoomX = Zoom.Get().X;
		const auto& SongsMap = SequenceData->HarmonixMidiFile->GetSongMaps();
		const auto& SectionStartTime = SongsMap->TickToMs(Section->Clip->StartTick);
		const auto& SectionEndTime = SongsMap->TickToMs(Section->Clip->EndTick);



		const float SectionDuration = (SectionEndTime - SectionStartTime) * ZoomX;
		//UE_LOG(LogTemp, Warning, TEXT("Section Duration %f"), SectionDuration);
		auto SectionGeometry = AllottedGeometry.MakeChild(
			FVector2f((SectionStartTime - Position.Get().X) * ZoomX, 0),
			FVector2f(SectionDuration, AllottedGeometry.Size.Y)
		);



		auto SectionCullingRect = MyCullingRect.IntersectionWith(FSlateRect::FromPointAndExtent(SectionGeometry.LocalToAbsolute(FVector2D(0, 0)), SectionGeometry.Size));

		LayerId = Section->OnPaint(Args, SectionGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	return LayerId;
}
