// Fill out your copyright notice in the Description page of Project Settings.


#include "M2SoundEdGraphSchema.h"
#include "Interfaces/unDAWMetasoundInterfaces.h"
#include "AudioParameter.h"
#include "IAudioParameterInterfaceRegistry.h"
#include "SGraphNode.h"
#include "Sound/SoundBase.h"
#include "EditorSlateWidgets/SM2SoundEdGraphNode.h"
#include "EditorSlateWidgets/SM2AudioOutputNode.h"
#include "EditorSlateWidgets/SM2MidiTrackGraphNode.h"


const FPinConnectionResponse UM2SoundEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (A->Direction == B->Direction)
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect output to output or input to input."));
	
	if (A->PinType.PinCategory == "Track" && B->PinType.PinCategory == "Track")
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Connect Track Bus"));
	}

	//deal with pin categories - Track-Audio and Track-Midi
	if (A->PinType.PinCategory == "Track-Audio" && B->PinType.PinCategory == "Track-Audio")
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Connect Track to Audio"));
	}

	if (A->PinType.PinCategory == "Track-Midi" && B->PinType.PinCategory == "Track-Midi")
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Connect Track to Midi"));
	}


	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not implemented by this schema."));
}

void UM2SoundEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UEdGraphSchema::GetGraphContextActions(ContextMenuBuilder);

	if(auto& FromPin = ContextMenuBuilder.FromPin)
	{
		if (FromPin->PinType.PinCategory == "Track-Midi")
		{
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewInstrument>());
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioOutput>());
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioInsert>());
		}

		if (FromPin->PinType.PinCategory == "Track-Audio")
		{
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioOutput>());
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioInsert>());
		}
	}

	if (!ContextMenuBuilder.FromPin)
	{
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewOutput>());
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewInstrument>());
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioOutput>());
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioInsert>());
	}
}

FM2SoundGraphAddNodeAction::FM2SoundGraphAddNodeAction(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping, const int32 InSectionID, const int32 InSortOrder)
	: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, 0)
{

}

UEdGraphNode* FM2SoundGraphAddNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const FScopedTransaction Transaction(TransactionText);
	ParentGraph->Modify();
	Cast<UM2SoundGraph>(ParentGraph)->GetSequencerData()->Modify();
	UEdGraphNode* NewNode = MakeNode(ParentGraph, FromPin);
	NewNode->NodePosX = Location.X + LocationOffset.X;
	NewNode->NodePosY = Location.Y + LocationOffset.Y;
	if (FromPin) for (UEdGraphPin* Pin : NewNode->Pins)
	{
		if (FromPin->Direction == Pin->Direction) continue;
		ParentGraph->GetSchema()->TryCreateConnection(Pin, FromPin);
		NewNode->NodeConnectionListChanged();
		FromPin->GetOwningNode()->NodeConnectionListChanged();
		break;
	}

	//NewNode->bHasCompilerMessage = true;
	//NewNode->GetDeprecationResponse()->Message = FText::FromString("This node is deprecated and will be removed in a future version of the plugin.");

	NewNode->GetGraph()->NotifyGraphChanged();

	return NewNode;
}


void UM2SoundGraphMidiOutputNode::AllocateDefaultPins()
{
	// Create any pin for testing
	CreatePin(EGPD_Input, "Track-Midi", FName("Track (Midi)", 0));
	Pins.Last()->DefaultValue = "Default";
}

void UM2SoundGraphMidiOutputNode::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	if (ContextMenuBuilder.FromPin &&
		ContextMenuBuilder.FromPin->Direction != EGPD_Output) return;
	//ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction>());
}

void UM2SoundGraphMidiOutputNode::NodeConnectionListChanged()
{

	if (!Pins[0]->HasAnyConnections())
	{

		bHasCompilerMessage = true;
		ErrorType = EMessageSeverity::Info;
		ErrorMsg = FString("MIDI output vertex has no inputs");
		bHasChanges = true;
	}
	else {
		ClearCompilerMessage();
		ErrorMsg = FString("");
		bHasChanges = true;
	}

	UM2SoundEdGraphNodeConsumer::NodeConnectionListChanged();

}

TSharedPtr<class SGraphNode> FM2SoundGraphPanelNodeFactory::CreateNode(UEdGraphNode* InNode) const
{
	return nullptr;
}

FM2SoundGraphToOutputAction::FM2SoundGraphToOutputAction(const TArray<UEdGraphPin*>& InSourcePins) : FEdGraphSchemaAction(INVTEXT("Meta"), INVTEXT("To Output"), INVTEXT("Creates a M2Sound output for each selected output pin."), 0)
, SourcePins(InSourcePins)
{}

TArray<UEdGraphPin*> UM2SoundGraph::GetSelectedPins(EEdGraphPinDirection Direction) const
{
	TArray<UEdGraphPin*> Pins;

	return MoveTemp(Pins);
}

void UM2SoundGraph::AutoConnectTrackPinsForNodes(UM2SoundEdGraphNode& A, UM2SoundEdGraphNode& B)
{

	UEdGraphPin* AOutputPin = nullptr;

	auto IsPinValidCategory = [](UEdGraphPin* Pin)
	{
		return Pin->PinType.PinCategory == "Track" || Pin->PinType.PinCategory == "Track-Audio" || Pin->PinType.PinCategory == "Track-Midi";
	};
	
	for(UEdGraphPin* Pin : A.Pins)
	{
		
		if (Pin->Direction == EGPD_Output && IsPinValidCategory(Pin))
		{
			AOutputPin = Pin;
		}
	}

	if(!AOutputPin) return;

	for(UEdGraphPin* Pin : B.Pins)
	{
		if (Pin->Direction == EGPD_Input && IsPinValidCategory(Pin))
		{
			bool bSuccess = GetSchema()->TryCreateConnection(AOutputPin, Pin);
			if(bSuccess)
			{
				//A.NodeConnectionListChanged();
				//B.NodeConnectionListChanged();
			}
		}
	}


	
}

template<class T>
inline UM2SoundEdGraphNode* UM2SoundGraph::CreateNodeForVertexClass(int ColumnPosition, int VerticalPosition, UM2SoundEdGraphNode* InputNode)
{

	
	FGraphNodeCreator<T> NodeCreator(*this);
	T* Node = NodeCreator.CreateNode();
	//Node->Vertex = NewObject<T>(Node->GetSequencerData(), NAME_None, RF_Transactional);
	Node->NodePosX = ColumnPosition;
	Node->NodePosY = VerticalPosition;
	NodeCreator.Finalize();

	//log position and index for debugging
	//UE_LOG(LogTemp, Warning, TEXT("Index: %d, RowIndex: %d, ColumnPosition: %d"), Index, VerticalPosition, ColumnPosition);

	AutoConnectTrackPinsForNodes(*InputNode, *Node);

	return Node;
}


//The initialize graph is not really safe to be called other than through the initialization sequence of the m2sound asset
void UM2SoundGraph::InitializeGraph()
{
	Nodes.Empty();

	const int OffsetBetweenNodes = 200;
	const int OutputsColumnPosition = 1400;
	const int InputsColumnPosition = 300;
	const int MidiOutsColumnPosition = 800;
	const int InstrumentColumns = 950;

	auto AudioOutputPlacement = [&] (int RowIndex)
	{
		return RowIndex * OffsetBetweenNodes;
	};

	auto MidiOutputPlacement = [&](int RowIndex)
		{
			return RowIndex * OffsetBetweenNodes + OffsetBetweenNodes / 2;
		};


	int i = 0;


	for(const auto& [index, Track] : GetSequencerData()->TrackInputs)
	{
		FGraphNodeCreator<UM2SoundGraphInputNode> NodeCreator(*this);
		UM2SoundGraphInputNode* Node = NodeCreator.CreateNode();

		Node->Vertex = Track;
		Node->TrackId = index;
		Node->AssignedTrackId = index;
		Node->Name = FName(GetSequencerData()->GetTracksDisplayOptions(Node->TrackId).trackName);
		//Node->GetNodeTitle(ENodeTitleType::FullTitle).SetUseLargeFont(true);
		Node->NodePosX = InputsColumnPosition;
		Node->NodePosY = i * OffsetBetweenNodes;
		NodeCreator.Finalize();

		FGraphNodeCreator<UM2SoundPatchContainerNode> InstrumentNodeCreator(*this);
		UM2SoundPatchContainerNode* InstrumentNode = InstrumentNodeCreator.CreateNode();
		//InstrumentNode->InterfaceClass = unDAW::Metasounds::FunDAWInstrumentRendererInterface;

		auto& InstrumentVertex = Track->Outputs[0];

		InstrumentNode->Vertex = InstrumentVertex;
		InstrumentNode->NodePosX = InstrumentColumns;
		InstrumentNode->NodePosY = i * OffsetBetweenNodes;
		
		InstrumentNodeCreator.Finalize();

		TArray<UM2SoundEdGraphNode*> Outputs;

		for(auto& Output : InstrumentVertex->Outputs)
		{
			//CreateNodeForVertexClass<Output>(i, OutputsColumnPosition, AudioOutputPlacement(i));
			if(Output->IsA<UM2SoundAudioOutput>())
			{
				Outputs.Add(CreateNodeForVertexClass<UM2SoundGraphAudioOutputNode>(OutputsColumnPosition, AudioOutputPlacement(i), InstrumentNode));
				
			}

			if(Output->IsA<UM2SoundMidiOutput>())
			{
				Outputs.Add(CreateNodeForVertexClass<UM2SoundGraphMidiOutputNode>(MidiOutsColumnPosition, MidiOutputPlacement(i), Node));
			}
			Outputs.Last()->Vertex = Output;
		}

		AutoConnectTrackPinsForNodes(*Node, *InstrumentNode);
		

		Node->VertexUpdated();
		InstrumentNode->VertexUpdated();

		for (auto Output : Outputs)
		{
			AutoConnectTrackPinsForNodes(*InstrumentNode, *Output);
			Output->VertexUpdated();
		}

		i++;
	}

	PerformVertexToNodeBinding();
	NotifyGraphChanged();

	GetSequencerData()->OnVertexAdded.AddUniqueDynamic(this, &UM2SoundGraph::OnVertexAdded);
}

void UM2SoundGraph::PerformVertexToNodeBinding()
{
	for (const auto NodeAsObject : Nodes)
	{
		auto Node = Cast<UM2SoundEdGraphNode>(NodeAsObject);
		if(!Node) continue;

		UM2SoundVertex* Vertex = Node->Vertex;
		if (!Vertex) continue;

		Vertex->OnVertexUpdated.AddUniqueDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	}

}

UEdGraphNode* FM2SoundGraphAddNodeAction_NewOutput::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	FGraphNodeCreator<UM2SoundGraphMidiOutputNode> NodeCreator(*ParentGraph);
	UM2SoundGraphMidiOutputNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Output");

	Node->Vertex = NewObject<UM2SoundMidiOutput>(Node->GetSequencerData(),NAME_None, RF_Transactional);
	Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);
	NodeCreator.Finalize();


	
	return Node;
}

FText UM2SoundGraphConsumer::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	if (Pin->Direction == EGPD_Output) return FText::FromName(Pin->PinName);
	int32 Index = Pins.Find(const_cast<UEdGraphPin*>(Pin));
	check(Index != INDEX_NONE && Vertex);
	//if (Index >= Vertex->GetInputInfo().Num()) return INVTEXT("This pin should not exist! Remove and re-add the node.");
	//return Vertex->GetInputInfo()[Index].DisplayName;

	return INVTEXT("Le Poop");
}

void UM2SoundGraphInputNode::AllocateDefaultPins()
{
	// create midi track output pin
	CreatePin(EGPD_Output, "Track-Midi", FName("Track (Midi)", 0));

	//CreatePin(EGPD_Output, "Track", FName("Track", 0));
	Pins.Last()->DefaultValue = "Default";
}

TSharedPtr<SGraphNode> UM2SoundGraphInputNode::CreateVisualWidget()
{
	return SNew(SM2MidiTrackGraphNode, this);
}

inline TSharedPtr<SGraphNode> UM2SoundPatchContainerNode::CreateVisualWidget()
{
	return SNew(SM2SoundPatchContainerGraphNode<unDAW::Metasounds::FunDAWInstrumentRendererInterface>, this);
}

void UM2SoundPatchContainerNode::AllocateDefaultPins()
{
	// Create any pin for testing
	CreatePin(EGPD_Input, "Track-Midi", FName("Track (Midi)", 0));
	Pins.Last()->DefaultValue = "Default";

	CreatePin(EGPD_Output, "Track-Audio", FName("Track (Audio)", 0));
	Pins.Last()->DefaultValue = "Default";


}

UEdGraphNode* FM2SoundGraphAddNodeAction_NewInstrument::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	FGraphNodeCreator<UM2SoundPatchContainerNode> NodeCreator(*ParentGraph);
	UM2SoundPatchContainerNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Instrument");
	NodeCreator.Finalize();

	//Node->Err

	Node->Vertex = NewObject<UM2SoundPatch>(Node->GetSequencerData(), NAME_None, RF_Transactional);
	Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);
	return Node;
}

void UM2SoundEdGraphNode::NodeConnectionListChanged()
{
	return;
	
	UEdGraphNode::NodeConnectionListChanged();

	//if no vertex probably recreating graph idk. 
	if(!Vertex) return;

	UE_LOG(LogTemp, Warning, TEXT("m2sound graph schema: NodeConnectionListChanged, %s"), *Vertex->GetName());

	//update CurrentTrackOuputs
	CurrentTrackOutputs.Empty();
	TArray<UM2SoundEdGraphNode*> Outputs;
	UM2SoundEdGraphNode* FoundTrackInput = nullptr;

	//GetInputs

	for (auto Pin : Pins)
	{
		if (Pin->Direction == EGPD_Output && (Pin->PinType.PinCategory == "Track-Midi" || Pin->PinType.PinCategory == "Track-Audio"))
		{
			//if Pin has connections, cast them to M2SoundEdGraphNode and add them to CurrentTrackOutputs
			if (Pin->LinkedTo.Num() > 0)
			{
				for (auto LinkedPin : Pin->LinkedTo)
				{
					if (LinkedPin->GetOwningNode()->IsA<UM2SoundEdGraphNode>())
					{
						Outputs.Add(Cast<UM2SoundEdGraphNode>(LinkedPin->GetOwningNode()));
					}
				}

			}
		}

		

		//find if track input is connected
		if (Pin->Direction == EGPD_Input && (Pin->PinType.PinCategory == "Track-Midi" || Pin->PinType.PinCategory == "Track-Audio"))
		{
			if (Pin->LinkedTo.Num() > 0)
			{
				for (auto LinkedPin : Pin->LinkedTo)
				{
					if (LinkedPin->GetOwningNode()->IsA<UM2SoundEdGraphNode>())
					{
						FoundTrackInput = Cast<UM2SoundEdGraphNode>(LinkedPin->GetOwningNode());
					}
				}

			}
		}

		


	}

	//if meaningful connections (only track input is relevant here?) changed, inform the vertex so that the builder can be triggered



	for (auto Output : Outputs)
	{
		CurrentTrackOutputs.AddUnique(Output);
	}



	//check if we have a track input
	//if(FoundTrackInput)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Found Track Input, my vertex is %s"), *Vertex->GetName());
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("No Track Input Found, my vertex is %s"), *Vertex->GetName());
	//	Vertex->BreakTrackInputConnection();
	//}


	GetGraph()->NotifyGraphChanged();
}

void UM2SoundEdGraphNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	//just print the pin data for now please
	if(!Pin) return;
	if(!Vertex) return;

	auto PinName = Pin->GetName();
	auto PinLinkedTo = Pin->LinkedTo.Num();
	auto PinDirection = Pin->Direction == EGPD_Input ? FString(TEXT("Input")) : FString(TEXT("Output"));
	UE_LOG(LogTemp, Warning, TEXT("m2sound graph schema: PinConnectionListChanged, %s, %s, %d"), *PinName, *PinDirection, PinLinkedTo);

	//if we're either of the track input pins, we need to check if we have a connection
	if(Pin->Direction == EGPD_Input && (Pin->PinType.PinCategory == "Track-Midi" || Pin->PinType.PinCategory == "Track-Audio"))
	{
		if (PinLinkedTo == 0)
		{
			Vertex->BreakTrackInputConnection();
			AssignedTrackId = INDEX_NONE;

		}
		if(PinLinkedTo > 0)
		{
			//if we have a connection, we need to find the node that we're connected to
			//and then we need to find the vertex that the node is connected to
			//and then we need to connect our vertex to that vertex
			//this is a bit of a mess, but it's the only way to do it right now
			//we could probably do this in the graph, but it's probably better if we just break inputs, and let the vertex handle the rest
			// still not sure how this would work when we have an external (in game) editor... but we'll get there.
			auto LinkedToPin = Pin->LinkedTo[0];
			auto LinkedToNode = LinkedToPin->GetOwningNode();
			auto AsM2Node = Cast<UM2SoundEdGraphNode>(LinkedToNode);
			auto LinkedToVertex = AsM2Node->Vertex;
			Vertex->MakeTrackInputConnection(LinkedToVertex);
			AssignedTrackId = AsM2Node->AssignedTrackId;
		}
		UpdateDownstreamTrackAssignment(AssignedTrackId);
		GetGraph()->NotifyGraphChanged();
	}

	// it's probably better if we just break inputs, and let the vertex handle the rest
	// still not sure how this would work when we have an external (in game) editor... but we'll get there.


}



bool CheckForErrorMessagesAndReturnIfAny(const UM2SoundEdGraphNode* Node, FString& OutErrorMsg)
{
	//this would be the errors from the interfaces and the such
	if(Node->Vertex->bHasErrors)
	{
		OutErrorMsg = Node->Vertex->VertexErrors;
		return true;
	}
	
	//now we traverse the builder results and check for errors
	//EMetaSoundBuilderResult

	bool bHasErrors = false;

	for(const auto& [ErrName, Result] : Node->Vertex->BuilderResults)
	{
		if(Result == EMetaSoundBuilderResult::Failed)
		{
			OutErrorMsg += ErrName.ToString() + "- Builder Operation Failed\n";
			bHasErrors = true;
		}
	}

	return bHasErrors;
}


void UM2SoundEdGraphNode::VertexUpdated()
{
	UE_LOG(LogTemp, Warning, TEXT("m2sound graph schemca: VertexUpdated, %s has %d outputs"), *Vertex->GetName(), Vertex->Outputs.Num());


	//assuming each vertex only implements one interface, we can keep most of the logic here
	Audio::FParameterInterfacePtr interface = nullptr;

	if(IsA<UM2SoundPatchContainerNode>())
	{
		interface = unDAW::Metasounds::FunDAWInstrumentRendererInterface::GetInterface();

	}

	if (IsA<UM2SoundAudioInsert>() || IsA<UM2SoundPatchContainerNode>())
	{
		auto AsPatch = Cast<UM2SoundPatch>(Vertex);
		//if patch is valid, get its name and give it to this node
		if(AsPatch->Patch)
		{
			Name = FName(AsPatch->Patch->GetName());
		}
		else
		{
			Name = "No Patch Selected";
		}
	}

	bHasCompilerMessage = CheckForErrorMessagesAndReturnIfAny(this, ErrorMsg);

	//clear autogen pins
	for(auto& Pin : AutoGeneratedPins)
	{
		Pin->BreakAllPinLinks();
		RemovePin(Pin);
	}

	AutoGeneratedPins.Empty();

	bool bShowAdvanced = false;

	if (!bShowAdvanced)
	{
		GetGraph()->NotifyGraphChanged();
		return;
	}
	//create pins for meta sound ios
	for(auto& VertexMetasoundOutput : Vertex->MetasoundOutputs)
	{
		bool bBelongsToInterface = false;

		if(interface)
		{
			TArray<Audio::FParameterInterface::FOutput> FoundMatchingOutputs =
				interface->GetOutputs().FilterByPredicate([&VertexMetasoundOutput](const Audio::FParameterInterface::FOutput& Output)
				{
					return Output.ParamName == VertexMetasoundOutput.PinName;
				});

			if(FoundMatchingOutputs.Num() > 0)
			{
				bBelongsToInterface = true;
			}
		}
		if(!bBelongsToInterface)
		{
			CreatePin(EGPD_Output, VertexMetasoundOutput.DataType, VertexMetasoundOutput.PinName);
			//Pins.Last()->DefaultValue = VertexMetasoundOutput.Key;
			AutoGeneratedPins.Add(Pins.Last());
		}
		

	}

	for(auto& VertexMetasoundInput : Vertex->MetasoundInputs)
	{
		bool bBelongsToInterface = false;

		if(interface)
		{
			TArray<Audio::FParameterInterface::FInput> FoundMatchingInputs =
				interface->GetInputs().FilterByPredicate([&VertexMetasoundInput](const Audio::FParameterInterface::FInput& Input)
				{
					return Input.InitValue.ParamName == VertexMetasoundInput.PinName;
				});

			if(FoundMatchingInputs.Num() > 0)
			{
				bBelongsToInterface = true;
			}
		}
		if(!bBelongsToInterface)
		{
			CreatePin(EGPD_Input, VertexMetasoundInput.DataType, VertexMetasoundInput.PinName);
			//Pins.Last()->DefaultValue = VertexMetasoundInput.Key;
			AutoGeneratedPins.Add(Pins.Last());
		}

		
	}

	GetGraph()->NotifyGraphChanged();
	
}

void UM2SoundGraphAudioOutputNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "Track-Audio", FName("Track (Audio)", 0));
	Pins.Last()->DefaultValue = "Default";
}

TSharedPtr<SGraphNode> UM2SoundGraphAudioOutputNode::CreateVisualWidget()
{
	return SNew(SM2AudioOutputNode, this);
}

//TSharedPtr<SGraphNode> UM2SoundGraphAudioOutputNode::CreateVisualWidget()
//{
//	return SNew(SGraphNode)
//		.WithPinLabels(false)
//		.WithPinDefaultValues(false);
//
//		
//}

UEdGraphNode* FM2SoundGraphAddNodeAction_NewAudioOutput::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	FGraphNodeCreator<UM2SoundGraphAudioOutputNode> NodeCreator(*ParentGraph);
	UM2SoundGraphAudioOutputNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Audio Output");

	Node->Vertex = NewObject<UM2SoundAudioOutput>(Node->GetSequencerData(), NAME_None, RF_Transactional);
	Node->GetSequencerData()->AddVertex(Node->Vertex);
	NodeCreator.Finalize();

	return Node;
}

TSharedPtr<SGraphNode> UM2SoundAudioInsertNode::CreateVisualWidget()
{
	return SNew(SM2SoundPatchContainerGraphNode<unDAW::Metasounds::FunDAWCustomInsertInterface>, this);
}

void UM2SoundAudioInsertNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "Track-Audio", FName("Track (Audio)", 0));
	Pins.Last()->DefaultValue = "Default";

	CreatePin(EGPD_Output, "Track-Audio", FName("Track (Audio)", 0));
	Pins.Last()->DefaultValue = "Default";

}


UEdGraphNode* FM2SoundGraphAddNodeAction_NewAudioInsert::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	UEdGraphPin* FromPinConnection = nullptr;
	TArray<UM2SoundEdGraphNode*> Pins;
	if (FromPin)
	{
		UM2SoundEdGraphNode* AsM2SoundNode = Cast<UM2SoundEdGraphNode>(FromPin->GetOwningNode());
		if(AsM2SoundNode)
		{
			Pins = AsM2SoundNode->CurrentTrackOutputs;
		}
	}

	if(Pins.Num() > 0)
	{
		//FromPinConnection = Pins[0];
	}

	FGraphNodeCreator<UM2SoundAudioInsertNode> NodeCreator(*ParentGraph);
	UM2SoundAudioInsertNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Audio Insert");

	auto NewPatchVertex = NewObject<UM2SoundPatch>(Node->GetSequencerData(), NAME_None, RF_Transactional);
	//Node->GetSequencerData()->AddVertex(Node->Vertex);
	Node->Vertex = NewPatchVertex;

	//assign default passthrough patch for audio insert 
	//'/unDAW/Patches/System/unDAW_PassThroughInsert.unDAW_PassThroughInsert'

	auto DefaultPatchTest = FSoftObjectPath(TEXT("'/unDAW/Patches/System/unDAW_PassThroughInsert.unDAW_PassThroughInsert'"));
	NewPatchVertex->Patch = CastChecked<UMetaSoundPatch>(DefaultPatchTest.TryLoad());

	UM2SoundEdGraphNode* FoundTrackInput = nullptr;
	UEdGraphPin* FoundTrackInputPin = nullptr;

	if (FromPinConnection)
	{
		//get the vertex that the input-vertex is CURRENTLY connected to, we need to connect the new patch to this vertex

		FoundTrackInput = Cast<UM2SoundEdGraphNode>(FromPinConnection->GetOwningNode());
		auto M2SoundVertex = FoundTrackInput->Vertex;
	}
	//auto ParentGraph = Cast<UM2SoundGraph>(ParentGraph);
	
	Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);

	NodeCreator.Finalize();

	ParentGraph->NotifyGraphChanged();

	return Node;
}
