// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "M2SoundGraphData.h"
#include "M2SoundGraphRenderer.h"
#include <EdGraphUtilities.h>
#include <EdGraph/EdGraphNode.h>
#include "SGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "M2SoundEdGraphSchema.h"
#include "Vertexes/M2SoundVertex.h"

#include "M2SoundEdGraphNodeBaseTypes.generated.h"


DECLARE_DELEGATE(FOnNodeUpdated);

UCLASS()
class BK_EDITORUTILITIES_API UM2SoundEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()
public:

	FOnNodeUpdated OnNodeUpdated;

	void NodeConnectionListChanged() override;

	void PinConnectionListChanged(UEdGraphPin* Pin) override;

	bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override { return Schema->IsA(UM2SoundEdGraphSchema::StaticClass()); }
	bool IncludeParentNodeContextMenu() const override { return true; }
	UM2SoundGraph* GetGraph() const { return Cast<UM2SoundGraph>(UEdGraphNode::GetGraph()); }
	UDAWSequencerData* GetSequencerData() const { return GetGraph()->GetSequencerData(); }

	//FLinearColor GetNodeTitleColor() const override { return FColor(23, 23, 23, 23); }
	FLinearColor GetNodeBodyTintColor() const override { return FColor(220, 220, 220, 220); }

	UPROPERTY()
	bool bShowAdvanced = false;

	UFUNCTION()
	virtual void VertexUpdated();

	UPROPERTY(EditAnywhere, Category = "Node")
	FName Name;

	UPROPERTY(EditAnywhere)
	UM2SoundVertex* Vertex;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromName(Name); }

	UPROPERTY(VisibleAnywhere)
	int AssignedTrackId = INDEX_NONE;

	FLinearColor GetNodeTitleColor() const override { return GetSequencerData()->GetTracksDisplayOptions(Vertex->TrackId).trackColor; }

	TArray<UEdGraphPin*> AutoGeneratedPins;

	// used to automatically resolve the track assignment of the node when adding inserts
	UPROPERTY(VisibleAnywhere, Category = "M2Sound Node")
	TArray<UM2SoundEdGraphNode*> CurrentTrackOutputs;

	UM2SoundEdGraphNode* CurrentTrackInput;

	void UpdateDownstreamTrackAssignment(int NewTrackId) {
		AssignedTrackId = NewTrackId;
		for (UEdGraphPin* Pin : Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == "Track")
			{
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					if (UM2SoundEdGraphNode* LinkedNode = Cast<UM2SoundEdGraphNode>(LinkedPin->GetOwningNode()))
					{
						LinkedNode->UpdateDownstreamTrackAssignment(NewTrackId);
					}
				}
			}
		}
	}

protected:

	bool bHasChanges = false;
};

//The consumer base node is the base class for all nodes that are downstream from the track,
UCLASS()
class BK_EDITORUTILITIES_API UM2SoundEdGraphNodeConsumer : public UM2SoundEdGraphNode
{
	GENERATED_BODY()
public:

	void NodeConnectionListChanged() override {
		UM2SoundEdGraphNode::NodeConnectionListChanged();
		//SyncModelConnections();
		int CurrentTrackId = AssignedTrackId;
		if (const auto UpstreamSource = GetUpstreamNode())
		{
			AssignedTrackId = UpstreamSource->AssignedTrackId;
		}
		else {
			AssignedTrackId = INDEX_NONE;
		}

		if (CurrentTrackId != AssignedTrackId)
		{
			//SyncModelConnections();
		}

		UpdateDownstreamTrackAssignment(AssignedTrackId);
	}

	const UM2SoundEdGraphNode* GetUpstreamNode() const {
		for (UEdGraphPin* Pin : Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
			{
				return Cast<UM2SoundEdGraphNode>(Pin->LinkedTo[0]->GetOwningNode());
			}
		}
		return nullptr;

		//FLinearColor GetNodeTitleColor() const override { return TitleColor; }
	};
};

//represents a MIDI output that will be listed in the Listener component
UCLASS()
class UM2SoundGraphMidiOutputNode : public UM2SoundEdGraphNodeConsumer
{
	GENERATED_BODY()

public:

	// This is the name of the output node, which will be used to identify the output to gameplay listener components
	UPROPERTY(EditAnywhere, Category = "M2Sound Node")
	FName OutputName;

	// if true, the name will be automatically generated from the name of the connected track
	UPROPERTY(EditAnywhere, Category = "M2Sound Node")
	bool AutoName;

	void AllocateDefaultPins() override;
	void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

	void NodeConnectionListChanged() override;

	//	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(FString::Printf(TEXT("MidiOut: %s"), *Cast<UM2SoundMidiOutput>(Vertex)->OutputName.ToString())); }
};

//represents an audio output that will be auto connected and managed by the performer component, will also have a fancy knob to control volume
UCLASS()
class UM2SoundGraphAudioOutputNode : public UM2SoundEdGraphNodeConsumer
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "M2Sound Node")
	float Gain = 1.0f;

	UM2SoundAudioOutput* AsOutputVertex = nullptr;

	void AllocateDefaultPins() override;

	TSharedPtr<SGraphNode> CreateVisualWidget() override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(FString::Printf(TEXT("Audio Out"))); }

	UFUNCTION()
	void SetOutputGain(float NewGain) {
		//UE_LOG(LogTemp, Warning, TEXT("Setting Gain to %f"), NewGain);
		Gain = NewGain;

		if (!AsOutputVertex)
		{
			//try cast vertex
			AsOutputVertex = Cast<UM2SoundAudioOutput>(Vertex);
		}

		if (AsOutputVertex)
		{
			AsOutputVertex->Gain = Gain;
			auto GainParamName = AsOutputVertex->GainParameterName;
			AsOutputVertex->TransmitAudioParameter(FAudioParameter(GainParamName, Gain));
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("No output vertex found"));
		}
	}
};

UCLASS()
class UM2SoundGraphInputNode : public UM2SoundEdGraphNode
{
	GENERATED_BODY()

public:

	// this number represents the track number in the sequencer
	UPROPERTY()
	int TrackId = INDEX_NONE;

	void AllocateDefaultPins() override;

	TSharedPtr<SGraphNode> CreateVisualWidget() override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(FString::Printf(TEXT("Track In: %s"), *Name.ToString())); }
};

UCLASS()
class BK_EDITORUTILITIES_API UM2SoundPatchContainerNode : public UM2SoundEdGraphNodeConsumer
{
	GENERATED_BODY()

public:

	UM2SoundPatchContainerNode() { bShowAdvanced = false; } //for now show advanced is false cause we don't do much with the pins

	// Returns true if it is possible to jump to the definition of this node (e.g., if it's a variable get or a function call)
	virtual bool CanJumpToDefinition() const override { return true; }

	// Jump to the definition of the MetaSound patch driving this node
	virtual void JumpToDefinition() const override {
		//cast vertex to patch vertex and get metasound patch reference
		UM2SoundPatch* Patch = Cast<UM2SoundPatch>(Vertex);
		UMetaSoundPatch* Object = Patch->Patch;
		//open asset editor for the metasound patch

		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
	}

	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;

	void AllocateDefaultPins() override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(FString::Printf(TEXT("Instrument: %s"), *Name.ToString())); }
};

UCLASS()
class BK_EDITORUTILITIES_API UM2SoundAudioInsertNode : public UM2SoundPatchContainerNode
{
	GENERATED_BODY()

public:

	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;

	void AllocateDefaultPins() override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(FString::Printf(TEXT("Insert: %s"), *Name.ToString())); }
};

UCLASS()
class UM2SoundGraphConsumer : public UM2SoundEdGraphNode
{
	GENERATED_BODY()
public:

	FText GetPinDisplayName(const UEdGraphPin* Pin) const override;
	//void AllocateDefaultPins() override;
	//void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	//void NodeConnectionListChanged() override;
	//void PostPasteNode() override;
	//void SyncModelConnections();
};

