// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorSlateWidgets/SSM2AudioOutputNode.h"
#include "SlateOptMacros.h"
#include "SAudioSlider.h"
#include "SAudioRadialSlider.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SM2AudioOutputNode::Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode)
{
	GraphNode = InGraphNode;
	OutputNode = Cast<UM2SoundGraphAudioOutputNode>(GraphNode);

	UpdateGraphNode();

}
TSharedRef<SWidget> SM2AudioOutputNode::CreateNodeContentArea()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)

		.Padding(FMargin(0, 3))
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.FillWidth(1.0f)
				[
					// LEFT
					SAssignNew(LeftNodeBox, SVerticalBox)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoWidth()

				[
					// Center
					SAssignNew(MainVerticalBox, SVerticalBox)
						+ SVerticalBox::Slot()

						//patch select

						.AutoHeight()
						[
							//make FAudioRadialSliderStyle

							SNew(SAudioRadialSlider)
								.SliderProgressColor_Lambda([this] {return GetSliderProgressColor(); })
								//_Lamba(this, &SM2AudioOutputNode::GetSliderProgressColor)
								//.Style(FAudioRadialSliderStyle())
						]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					// RIGHT
					SAssignNew(RightNodeBox, SVerticalBox)
				]
		];


}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

/** Constructs this widget with InArgs */


