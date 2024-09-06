#include "UndawMusicSequencer.h"
#include "SlateFwd.h"
#include "Components/Widget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"


void SUndawMusicSequencer::Construct(const FArguments& InArgs, UDAWSequencerData* InSequenceToEdit)
{
	SequenceData = InSequenceToEdit;

	const auto& SequenceName = SequenceData->GetFName();

	TimelineHeight = InArgs._TimelineHeight;

	ChildSlot[
		SAssignNew(ScrollBox, SScrollBox)
			+SScrollBox::Slot()
			[
				SAssignNew(Splitter, SSplitter)
					.Orientation(Orient_Vertical)
					.HitDetectionSplitterHandleSize(5.0f)
					.ResizeMode(ESplitterResizeMode::Fill)
			]
	];

	//CreateGridPanel();
	CreateScrollBox();
}

void SUndawMusicSequencer::CreateGridPanel()
{
	constexpr int TracksRow = 0;
	constexpr int LanesRow = 1;


	
	for (int i = 0; i < 20; i++)
	{
		SGridPanel::FSlot* TrackSlot;
		SGridPanel::FSlot* LaneSlot;

		GridPanel->AddSlot(TracksRow, i)
			.Expose(TrackSlot)
			[
				SNew(SBox)
					.WidthOverride(100)
					.HeightOverride(100)
					[
						SNew(SBorder)
							.BorderBackgroundColor(FLinearColor::Red)
							.Content()
							[
								SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("Track %d"), i)))
							]
					]

			];

		GridPanel->AddSlot(LanesRow, i)
			.Expose(LaneSlot)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Track %d"), i)))
			];

	

		TrackSlots.Add(TrackSlot);
		LaneSlots.Add(LaneSlot);

	}
}

void SUndawMusicSequencer::CreateScrollBox()
{

	//add 25 tracks
	for (int i = 0; i < 25; i++)
	{
		TSharedPtr<SDawSequencerTrackRoot> TrackRoot;
		Splitter->AddSlot(i)
			[
				SAssignNew(TrackRoot, SDawSequencerTrackRoot)

			];

		TrackRoot->OnVerticalMajorSlotResized.BindLambda([this](float InNewSize) { 
			UE_LOG(LogTemp, Warning, TEXT("Slot resized to %f"), InNewSize);
			for (auto& TrackRoot : TrackRoots) { TrackRoot->ResizeSplitter(InNewSize); }});

		TrackRoots.Add(TrackRoot);
		//TrackSlotsScrollBox.Add(TrackSlot);
	}

}

int32 SUndawMusicSequencer::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	auto TimelineGeometry = AllottedGeometry.MakeChild(
		FVector2f(150, 0),
		FVector2f(AllottedGeometry.Size.X - 150, TimelineHeight)
	);

	auto TimelineCullingRect = MyCullingRect.IntersectionWith(FSlateRect::FromPointAndExtent(TimelineGeometry.LocalToAbsolute(FVector2D(0, 0)), TimelineGeometry.Size));

	auto TrackAreaGeometry = AllottedGeometry.MakeChild(
		FVector2f(0, TimelineHeight),
		FVector2f(AllottedGeometry.Size.X, AllottedGeometry.Size.Y - TimelineHeight)
	);

	
	LayerId = PaintTimeline(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId);
	LayerId = PaintBackgroundGrid(Args, TrackAreaGeometry, MyCullingRect, OutDrawElements, LayerId);
	LayerId = ScrollBox->Paint(Args, TrackAreaGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	
	return LayerId;
}

int32 SUndawMusicSequencer::PaintBackgroundGrid(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	//just fill the background with a gray box
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		FAppStyle::GetBrush("Graph.Panel.SolidBackground"),
		ESlateDrawEffect::None,
		FLinearColor::Green.CopyWithNewOpacity(0.2f)
	);
	
	return LayerId;
}

int32 SUndawMusicSequencer::PaintTimeline(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	// first draw the timeline backgroumd a black box

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(150, 0), FVector2D(AllottedGeometry.Size.X, TimelineHeight)),
		FAppStyle::GetBrush("Graph.Panel.SolidBackground"),
		ESlateDrawEffect::None,
		FLinearColor::Black
	);

	// draw 30 vertical lines for fun, 
	for (int i = 0; i < 30; i++)
	{
		const float X = i * 100;
		const FVector2D Start(X, 0);
		const FVector2D End(X, TimelineHeight);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(150, 0), FVector2D(AllottedGeometry.Size.X, TimelineHeight)),
			{ Start, End },
			ESlateDrawEffect::None,
			FLinearColor::White
		);
	}

	
	return LayerId;
}

void SDawSequencerTrackRoot::Construct(const FArguments& InArgs)
{
	ChildSlot
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SDAwSequencerTrackControlsArea)
						.DesiredWidth(150)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SBox)
						[
							SNew(SBorder)
								.BorderBackgroundColor(FLinearColor::Green)
								.Content()
								[
									SNew(STextBlock)
										.Text(FText::FromString("Track Contents"))
								]
						]
				]
		];
}

void SDAwSequencerTrackControlsArea::Construct(const FArguments& InArgs)
{
	DesiredWidth = InArgs._DesiredWidth;

	ChildSlot
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
						.HeightOverride(100)
						.WidthOverride(DesiredWidth.Get())
						[
						SNew(SBorder)
							.BorderBackgroundColor(FLinearColor::Green)
							[
								SNew(STextBlock)
									.Text(FText::FromString("Controls"))
							]
						]
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
			UE_LOG(LogTemp, Warning, TEXT("Resizing"));
			bIsResizing = true;
			return FReply::Handled();
		}
	}


	
	return FReply::Unhandled();
}

FReply SDAwSequencerTrackControlsArea::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//resize the slot
	const FVector2D LocalMousePosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	//UE_LOG(LogTemp, Warning, TEXT("Mouse position %s, target val %f"), *LocalMousePosition.ToString(), MyGeometry.Size.X - 5);

	if (LocalMousePosition.X > MyGeometry.Size.X - 5)
	{
		
		bIsHoveringOverResizeArea = true;
	
	}
	else {
		bIsHoveringOverResizeArea = false;
	}
	
	return FReply::Unhandled();
}
