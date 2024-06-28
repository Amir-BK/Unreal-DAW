// Fill out your copyright notice in the Description page of Project Settings.

#include "M2SoundEdGraphSchema.h"
#include "Interfaces/unDAWMetasoundInterfaces.h"

#include "AudioParameter.h"
#include "IAudioParameterInterfaceRegistry.h"
#include "SGraphNode.h"
#include "Sound/SoundBase.h"
#include "M2SoundEdGraphNodeBaseTypes.h"
#include "EditorSlateWidgets/SM2SoundEdGraphNode.h"
#include "EditorSlateWidgets/SM2AudioOutputNode.h"
#include "UnDAWPreviewHelperSubsystem.h"
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

	//if Pin Category == MetasoundLiteral check Pin Subcategory

	if (A->PinType.PinCategory == "MetasoundLiteral" && B->PinType.PinCategory == "MetasoundLiteral")
	{
		//compare subcategories 
		if(A->PinType.PinSubCategory == B->PinType.PinSubCategory)
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Connect Metasound Literals, Type: ") + B->PinType.PinSubCategory.ToString());

			}
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not implemented by this schema."));
}

void UM2SoundEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UEdGraphSchema::GetGraphContextActions(ContextMenuBuilder);

	auto* AsM2SoundGraph = Cast<UM2SoundGraph>(ContextMenuBuilder.CurrentGraph);
	const auto& SequencerData = AsM2SoundGraph->GetSequencerData();

	TArray<FString> DummyInputs;

	for (const auto& Track : SequencerData->M2TrackMetadata)
	{
		DummyInputs.Add(Track.trackName);
	}

	for (const auto& [Name, Output] : SequencerData->CoreNodes.MappedOutputs)
	{
		DummyInputs.Add(Name.ToString());
	}

	if (auto& FromPin = ContextMenuBuilder.FromPin)
	{
		if (FromPin->PinType.PinCategory == "Track-Midi")
		{
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewInstrument>());
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioOutput>());
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioInsert>());

			for(auto Input : DummyInputs)
			{
				auto newAction = MakeShared<FM2SoundGraphAddNodeAction_NewGraphInputNode>();
				newAction->UpdateSearchData(FText::FromString(Input), INVTEXT("Midi Track"), INVTEXT("Midi Inputs"), INVTEXT("Midi"));
				
				ContextMenuBuilder.AddAction(newAction);
			}

		}

		if (FromPin->PinType.PinCategory == "Track-Audio")
		{
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioOutput>());
			ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioInsert>());

			if(FromPin->Direction == EGPD_Output)
			{
				ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewVariMixerNode>());
			}
		}
	}

	if (!ContextMenuBuilder.FromPin)
	{
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewOutput>());
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewInstrument>());
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioOutput>());
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewAudioInsert>());
		ContextMenuBuilder.AddAction(MakeShared<FM2SoundGraphAddNodeAction_NewVariMixerNode>());

		for (auto Input : DummyInputs)
		{
			auto newAction = MakeShared<FM2SoundGraphAddNodeAction_NewGraphInputNode>();
			newAction->UpdateSearchData(FText::FromString(Input),INVTEXT("Midi Track"), INVTEXT("Midi Inputs"), INVTEXT("Midi"));

			ContextMenuBuilder.AddAction(newAction);
		}
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
	//cast to m2sound node and bind the vertex to the node
	UM2SoundEdGraphNode* Node = Cast<UM2SoundEdGraphNode>(NewNode);
	if(IsValid(Node->Vertex))
	{
		Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	}
	//Node->Vertex->OnVertexUpdated.AddUniqueDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	NewNode->GetGraph()->NotifyGraphChanged();

	return NewNode;
}


FM2SoundGraphToOutputAction::FM2SoundGraphToOutputAction(const TArray<UEdGraphPin*>& InSourcePins) : FEdGraphSchemaAction(INVTEXT("Meta"), INVTEXT("To Output"), INVTEXT("Creates a M2Sound output for each selected output pin."), 0)
, SourcePins(InSourcePins)
{}

void UM2SoundGraph::SaveVertexRangesToCache()
{
	//we'll need only the patches

	for(const auto& Vertex : SelectedVertices)
	{
		
		
			auto asPatchVertex = Cast<UM2SoundPatch>(Vertex);

			if(asPatchVertex)
			{
				//get the patch vertex and get the default ranges
				//asPatchVertex->GetDefaultRanges();
				asPatchVertex->SaveDefaultsToVertexCache();

			}

		
	}
}

TArray<UEdGraphPin*> UM2SoundGraph::GetSelectedPins(EEdGraphPinDirection Direction) const
{
	TArray<UEdGraphPin*> Pins;

	return MoveTemp(Pins);
}

void UM2SoundGraph::AutoConnectTrackPinsForNodes(UM2SoundEdGraphNode& A, UM2SoundEdGraphNode& B)
{
	UEdGraphPin* AOutputPin = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("m2sound graph schema: AutoConnectTrackPinsForNodes, %s, %s"), *A.GetName(), *B.GetName());

	auto IsPinValidCategory = [](UEdGraphPin* Pin)
		{
			return Pin->PinType.PinCategory == "Track" || Pin->PinType.PinCategory == "Track-Audio" || Pin->PinType.PinCategory == "Track-Midi";
		};

	for (UEdGraphPin* Pin : A.Pins)
	{
		if (Pin->Direction == EGPD_Output && IsPinValidCategory(Pin))
		{
			AOutputPin = Pin;
		}
	}

	if (!AOutputPin) return;

	UE_LOG(LogTemp, Warning, TEXT("m2sound graph schema: Found A output Pin, %s, %s"), *AOutputPin->GetName(), *B.GetName());

	for (UEdGraphPin* Pin : B.Pins)
	{
		if (Pin->Direction == EGPD_Input && IsPinValidCategory(Pin))
		{
			bool bSuccess = GetSchema()->TryCreateConnection(AOutputPin, Pin);
			if (bSuccess)
			{
				A.NodeConnectionListChanged();
				B.NodeConnectionListChanged();
			}

			UE_LOG(LogTemp, Warning, TEXT("m2sound graph schema: Found B input Pin, %s, %s"), *Pin->GetName(), *B.GetName());
		}
	}


}





//The initialize graph is not really safe to be called other than through the initialization sequence of the m2sound asset
void UM2SoundGraph::InitializeGraph()
{
	Nodes.Empty();
	VertexToNodeMap.Empty();

	for (const auto& Vertex : GetSequencerData()->GetVertexes())
	{

		//create node in accordance to the vertex class, a little ugly but we have only three cases
		//for now check for the node class, we're looking for Patch Vertex, Audio Output Vertex and 'Input Handle Node' vertex

		if (Vertex->IsA<UM2SoundPatch>())
		{
			CreateDefaultNodeForVertex<UM2SoundPatchContainerNode>(Vertex, FPlacementDefaults::InstrumentColumns);
		}

		if(Vertex->IsA<UM2SoundAudioOutput>())
		{
			CreateDefaultNodeForVertex<UM2SoundGraphAudioOutputNode>(Vertex, FPlacementDefaults::OutputsColumnPosition);
		}

		if(Vertex->IsA<UM2SoundBuilderInputHandleNode>())
		{
			CreateDefaultNodeForVertex< UM2SoundGraphInputNode>(Vertex, FPlacementDefaults::InputsColumnPosition);
		}


	}

	for (const auto& [Vertex, Node] : VertexToNodeMap)
	{
		if(auto& InputVertex = Vertex->MainInput)
		{
			AutoConnectTrackPinsForNodes(*VertexToNodeMap[InputVertex], *Node);
			
		}
	}


	PerformVertexToNodeBinding();
	GetSequencerData()->OnVertexAdded.AddUniqueDynamic(this, &UM2SoundGraph::OnVertexAdded);
	NotifyGraphChanged();
}

void UM2SoundGraph::PerformVertexToNodeBinding()
{
	for (const auto NodeAsObject : Nodes)
	{
		auto Node = Cast<UM2SoundEdGraphNode>(NodeAsObject);
		if (!Node) continue;

		UM2SoundVertex* Vertex = Node->Vertex;
		if (!Vertex) continue;

		Vertex->OnVertexUpdated.AddUniqueDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
		Node->VertexUpdated();
	}
}

UEdGraphNode* FM2SoundGraphAddNodeAction_NewOutput::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	FGraphNodeCreator<UM2SoundGraphMidiOutputNode> NodeCreator(*ParentGraph);
	UM2SoundGraphMidiOutputNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Output");

	//Node->Vertex = NewObject<UM2SoundAudioOutput>(Node->GetSequencerData(),NAME_None, RF_Transactional);
	Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);
	NodeCreator.Finalize();

	return Node;
}

UEdGraphNode* FM2SoundGraphAddNodeAction_NewInstrument::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	FGraphNodeCreator<UM2SoundPatchContainerNode> NodeCreator(*ParentGraph);
	UM2SoundPatchContainerNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Instrument");
	NodeCreator.Finalize();

	//Node->Err
	auto DefaultPatchTest = FSoftObjectPath(TEXT("'/unDAW/Patches/System/unDAW_Fusion_Piano.unDAW_Fusion_Piano'"));

	auto* NewVertex	= NewObject<UM2SoundPatch>(Node->GetSequencerData(), NAME_None, RF_Transactional);
	NewVertex->Patch = CastChecked<UMetaSoundPatch>(DefaultPatchTest.TryLoad());
	Node->Vertex = NewVertex;
	Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);
	return Node;
}


UEdGraphNode* FM2SoundGraphAddNodeAction_NewAudioOutput::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	FGraphNodeCreator<UM2SoundGraphAudioOutputNode> NodeCreator(*ParentGraph);
	UM2SoundGraphAudioOutputNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Audio Output");

	Node->Vertex = NewObject<UM2SoundAudioOutput>(Node->GetSequencerData(), NAME_None, RF_Transactional);
	Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);

	Node->Vertex->SequencerData = Node->GetSequencerData();
	NodeCreator.Finalize();

	return Node;
}


UEdGraphNode* FM2SoundGraphAddNodeAction_NewAudioInsert::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	UEdGraphPin* FromPinConnection = nullptr;
	TArray<UM2SoundEdGraphNode*> Pins;
	UM2SoundVertex* M2SoundVertex = nullptr;
	if (FromPin)
	{
		UM2SoundEdGraphNode* AsM2SoundNode = Cast<UM2SoundEdGraphNode>(FromPin->GetOwningNode());
		if (AsM2SoundNode)
		{
			M2SoundVertex = AsM2SoundNode->Vertex;
		}
	}

	//print number of vertex outputs as warning
	if (M2SoundVertex)
	{
		UE_LOG(LogTemp, Warning, TEXT("m2sound graph schema: Vertex has %d outputs"), M2SoundVertex->Outputs.Num());
	}


	FGraphNodeCreator<UM2SoundAudioInsertNode> NodeCreator(*ParentGraph);
	UM2SoundAudioInsertNode* Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("Audio Insert");

	auto NewPatchVertex = FVertexCreator::CreateVertex<UM2SoundPatch>(Node->GetSequencerData());
	//Node->GetSequencerData()->AddVertex(Node->Vertex);
	//M2SoundVertex->Outputs.Empty();
	Node->Vertex = NewPatchVertex;
	//Node->Vertex->Outputs = M2SoundVertex->Outputs;
	//Node->Vertex->SequencerData = Node->GetSequencerData();

	//for(auto& Output : Node->Vertex->Outputs)
	//{
	//	Output->MainInput = Node->Vertex;
	//	Output->UpdateConnections();
	//}

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
		//auto M2SoundVertex = FoundTrackInput->Vertex;
	}
	//auto ParentGraph = Cast<UM2SoundGraph>(ParentGraph);

	//Node->GetSequencerData()->AddVertex(NewPatchVertex);

	//Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);

	NodeCreator.Finalize();

	ParentGraph->NotifyGraphChanged();

	return Node;
}

UEdGraphNode* FM2SoundGraphAddNodeAction_NewGraphInputNode::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	
	FGraphNodeCreator<UM2SoundGraphInputNode> NodeCreator(*ParentGraph);

	UM2SoundGraphInputNode* Node = NodeCreator.CreateUserInvokedNode();

	//acquire track metadata 
	const auto& SequencerData = Cast<UM2SoundGraph>(ParentGraph)->GetSequencerData();
	const auto& TrackMetadata = SequencerData->M2TrackMetadata;
	const auto& SearchString = GetMenuDescription().ToString();

	auto MyMetadata = TrackMetadata.IndexOfByPredicate([&SearchString](const FTrackDisplayOptions& Metadata)
		{
			return Metadata.trackName == SearchString;
		});

	if(MyMetadata != INDEX_NONE)
	{
		Node->TrackId = MyMetadata;
		Node->Name = FName(TrackMetadata[MyMetadata].trackName);
	}

	//auto NewInputVertex = NewObject<UM2SoundMidiInputVertex>(Node->GetSequencerData(), NAME_None, RF_Transactional);
	Node->Vertex = FVertexCreator::CreateVertex<UM2SoundMidiInputVertex>(Node->GetSequencerData());
	Node->Vertex->TrackId = Node->TrackId;
	//Node->Vertex->OnVertexUpdated.AddDynamic(Node, &UM2SoundEdGraphNode::VertexUpdated);
	Node->GetSequencerData()->AddVertex(Node->Vertex);

	NodeCreator.Finalize();

	ParentGraph->NotifyGraphChanged();
	//find the track metadata that corresponds to the input
	



	
	return Node;
}

UEdGraphNode* FM2SoundGraphAddNodeAction_NewVariMixerNode::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
	
	FGraphNodeCreator<UM2SoundVariMixerNode> NodeCreator(*ParentGraph);

	auto Node = NodeCreator.CreateUserInvokedNode();
	Node->Name = FName("VariMixer");

	NodeCreator.Finalize();
	
	return Node;
}
