// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "M2SoundGraphData.h"
#include <EdGraphUtilities.h>
#include <EdGraph/EdGraphNode.h>
#include "Components/AudioComponent.h"
#include "SGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "EdGraph/EdGraphSchema.h"
#include "Editor.h"
#include "M2SoundEdGraphSchema.h"
#include "Vertexes/M2SoundVertex.h"
#include "Editor.h"

#include "M2SoundEdGraphNodeBaseTypes.generated.h"

DECLARE_DELEGATE(FOnNodeUpdated);

#pragma region M2SoundEdGraphNodes

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
	virtual bool CanSplitPin(const UEdGraphPin* Pin) const override { return Pin->PinType.PinCategory == TEXT("Track-Audio"); }

	bool MakeConnection(UEdGraphPin* A, UEdGraphPin* B) const;

	const void SplitPin(const UEdGraphPin* Pin) const;

	void SetPinAsColorSource(UM2Pins* M2Pin);

	//void SyncToVertex() const;

	void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	//FLinearColor GetNodeTitleColor() const override { return FColor(23, 23, 23, 23); }
	FLinearColor GetNodeBodyTintColor() const override { return FColor(220, 220, 220, 220); }

	UEdGraphPin* ColorSourcePin = nullptr;

	//UFUNCTION()
	bool IsPinColorSource(const UEdGraphPin* Pin) const { return Pin == ColorSourcePin; }

	UPROPERTY()
	bool bShowAdvanced = false;

	UFUNCTION()
	virtual void VertexUpdated();

	UPROPERTY(EditAnywhere, Category = "Node")
	FName Name;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UM2SoundVertex> Vertex;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromName(Name); }

	void SyncVertexConnections() const;

	UPROPERTY(VisibleAnywhere)
	int AssignedTrackId = INDEX_NONE;

	FLinearColor GetNodeTitleColor() const override;

	TArray<UEdGraphPin*> AutoGeneratedPins;

	// used to automatically resolve the track assignment of the node when adding inserts
	UPROPERTY(VisibleAnywhere, Category = "M2Sound Node")
	TArray<TObjectPtr<UM2SoundEdGraphNode>> CurrentTrackOutputs;

	TObjectPtr<UM2SoundEdGraphNode> CurrentTrackInput;

	void AllocateDefaultPins() override;

	void UpdateDownstreamTrackAssignment(int NewTrackId) {
		AssignedTrackId = NewTrackId;
		//for (UEdGraphPin* Pin : Pins)
		//{
		//	if (Pin->Direction == EGPD_Output && (Pin->PinType.PinCategory == "Track-Midi" || Pin->PinType.PinCategory == "Track-Audio"))
		//	{
		//		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		//		{
		//			if (UM2SoundEdGraphNode* LinkedNode = Cast<UM2SoundEdGraphNode>(LinkedPin->GetOwningNode()))
		//			{
		//				LinkedNode->UpdateDownstreamTrackAssignment(NewTrackId);
		//			}
		//		}
		//	}
		//}

		//NodeConnectionListChanged();
	}

	void DestroyNode() override {
		UpdateDownstreamTrackAssignment(INDEX_NONE);

		Vertex->DestroyVertexInternal();
		Super::DestroyNode();
	}

protected:

	bool bHasChanges = false;
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Reroute"))
class BK_EDITORUTILITIES_API UM2SoundRerouteNode final : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UEdGraphPin* GetInputPin() const
	{
		return Pins[0];
	}

	UEdGraphPin* GetOutputPin() const
	{
		return Pins[1];
	}

	void AllocateDefaultPins() override
	{
		const FName InputPinName(TEXT("InputPin"));
		const FName OutputPinName(TEXT("OutputPin"));

		UEdGraphPin* MyInputPin = CreatePin(EGPD_Input, "wildcard", InputPinName);
		MyInputPin->bDefaultValueIsIgnored = true;

		UEdGraphPin* MyOutputPin = CreatePin(EGPD_Output, "wildcard", OutputPinName);
	}

	void NodeConnectionListChanged() override
	{
		//check if either pin is connected, if not, remove this node, if a pin is connected set category to match connected pin
		bool bHasConnections = false;

		if (GetInputPin()->HasAnyConnections())
		{
			bHasConnections = true;
			GetInputPin()->PinType.PinCategory = GetInputPin()->LinkedTo[0]->PinType.PinCategory;
			GetOutputPin()->PinType.PinCategory = GetInputPin()->LinkedTo[0]->PinType.PinCategory;
		}
		else if (GetOutputPin()->HasAnyConnections())
		{
			bHasConnections = true;
			GetOutputPin()->PinType.PinCategory = GetOutputPin()->LinkedTo[0]->PinType.PinCategory;
			GetInputPin()->PinType.PinCategory = GetOutputPin()->LinkedTo[0]->PinType.PinCategory;
		}

		if (!bHasConnections)
		{
			//set back to wildcard
			GetInputPin()->PinType.PinCategory = "wildcard";
			GetOutputPin()->PinType.PinCategory = "wildcard";
		}

		UEdGraphNode::NodeConnectionListChanged();
	}

	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override { OutInputPinIndex = 0; OutOutputPinIndex = 1; return true; }

	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
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

//represents an audio output that will be auto connected and managed by the performer component, will also have a fancy knob to control volume
UCLASS()
class UM2SoundGraphAudioOutputNode : public UM2SoundEdGraphNodeConsumer
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "M2Sound Node")
	float Gain = 1.0f;

	bool CanUserDeleteNode() const override { return false; }

	UM2SoundAudioOutput* AsOutputVertex = nullptr;

	TSharedPtr<SGraphNode> CreateVisualWidget() override;

	FLinearColor GetNodeTitleColor() const override { return FLinearColor::Black; }

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(FString::Printf(TEXT("Master Output"))); }

	UFUNCTION()
	void SetOutputGain(float NewGain) {
		Gain = NewGain;
		GetGraph()->GetSequencerData()->MasterOptions.MasterVolume = Gain;
		GetGraph()->GetSequencerData()->AuditionComponent->SetVolumeMultiplier(Gain);
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

	TSharedPtr<SGraphNode> CreateVisualWidget() override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(Cast<UM2SoundBuilderInputHandleVertex>(Vertex)->MemberName.ToString()); }
};

UCLASS()
class BK_EDITORUTILITIES_API UM2SoundPatchContainerNode : public UM2SoundEdGraphNodeConsumer
{
	GENERATED_BODY()

public:

	UM2SoundPatchContainerNode() { bShowAdvanced = true; } //for now show advanced is false cause we don't do much with the pins

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

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::FromString(FString::Printf(TEXT("Instrument: %s"), *Name.ToString())); }
};

UCLASS()
class BK_EDITORUTILITIES_API UM2SoundAudioInsertNode : public UM2SoundPatchContainerNode
{
	GENERATED_BODY()

public:

	UM2SoundAudioInsertNode() { bShowAdvanced = true; }

	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;

	//void AllocateDefaultPins() override;

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

UCLASS()
class UM2SoundDynamicGraphInputNode : public UM2SoundEdGraphNode
{
	GENERATED_BODY()

public:
	UM2SoundDynamicGraphInputNode() { bCanRenameNode = true; }

	void OnRenameNode(const FString& NewName) override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	//void AllocateDefaultPins() override;

};

UCLASS()
class UM2SoundVariMixerNode : public UM2SoundGraphConsumer
{
	GENERATED_BODY()
public:

	UM2SoundVariMixerNode() { bCanRenameNode = true; }

	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;

	//TSharedPtr<SM2VariMixerNode> MixerWidget;

	//void AllocateDefaultPins() override;

	void NodeConnectionListChanged() override;

	FLinearColor GetNodeTitleColor() const override { return FLinearColor::Black; }

	void OnRenameNode(const FString& NewName) override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
};

#pragma endregion M2SoundEdGraphNodes

#pragma region M2SoundEdGraphNodeActions

USTRUCT()
struct FM2SoundGraphSelectPinAsColorSourceAction : public FEdGraphSchemaAction
{
public:
	GENERATED_BODY()

	//FM2SoundGraphSelectPinAsColorSourceAction() : FEdGraphSchemaAction(INVTEXT(""), INVTEXT("Set as color source"), INVTEXT(""), INVTEXT("M2Data"), 0, 0) {}
	void PrintTestString() { UE_LOG(LogTemp, Warning, TEXT("Test String")); }
	//UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

#pragma endregion M2SoundEdGraphNodeActions