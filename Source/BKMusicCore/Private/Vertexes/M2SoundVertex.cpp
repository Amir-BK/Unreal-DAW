#include "Vertexes/M2SoundVertex.h"

#include "M2SoundGraphStatics.h"
#include "Metasound.h"
#include "Interfaces/unDAWMetasoundInterfaces.h"

#include "unDAWSettings.h"

DEFINE_LOG_CATEGORY(unDAWVertexLogs);

void UM2SoundPatch::SaveDefaultsToVertexCache()
{
	auto Config = FCachedVertexPinInfo();

	//for (const auto& [Name, Pin] : InPinsNew)
	//{
	//	Config.PinRanges.Add(Name, FFloatRange(Pin.MinValue, Pin.MaxValue));
	//}

	
	UUNDAWSettings::Get()->Cache.Add(Patch->GetFName(), Config);
	UUNDAWSettings::Get()->SaveConfig();
}

void UM2SoundVertex::PopulatePinsFromMetasoundData(const TArray<FMetaSoundBuilderNodeInputHandle>& InHandles, const TArray<FMetaSoundBuilderNodeOutputHandle>& OutHandles)
{
	EMetaSoundBuilderResult BuildResult;
	UMetaSoundSourceBuilder& BuilderContext = GetBuilderContext();
	MarkAllPinsStale();

	for(const auto& Handle : InHandles)
	{

		FName PinName;
		FName DataType;
		FName SearchName;
		BuilderContext.GetNodeInputData(Handle, PinName, DataType, BuildResult);
		auto LiteralValue = BuilderContext.GetNodeInputClassDefault(Handle, BuildResult);

		UM2Pins* PinObject = nullptr;
		

		using namespace M2Sound::Pins;
		{

			if (PinCategoryMap.Contains(PinName))
			{
				auto PinCategory = PinCategoryMap[PinName];
				SearchName = AutoDiscovery::AudioTrack;
				if (InputM2SoundPins.Contains(SearchName))
				{
					PinObject = InputM2SoundPins[SearchName];
					PinObject->bIsStale = false;
				}
				else
				{
					PinObject = CreateAudioTrackInputPin();
					InputM2SoundPins.Add(SearchName, PinObject);
				}

				auto* AsAudioTrackPin = Cast<UM2AudioTrackPin>(PinObject);


				switch(PinCategory)
				{
					case EVertexAutoConnectionPinCategory::AudioStreamL:
						if (IsValid(AsAudioTrackPin->AudioStreamL))
						{
							AsAudioTrackPin->AudioStreamL->SetHandle(Handle);
						}
						else
						{
							AsAudioTrackPin->AudioStreamL = CreateInputPin<UM2MetasoundLiteralPin>(Handle);
						}
						break;
					case EVertexAutoConnectionPinCategory::AudioStreamR:
						if (IsValid(AsAudioTrackPin->AudioStreamR))
						{
							AsAudioTrackPin->AudioStreamR->SetHandle(Handle);
						}
						else
						{
							AsAudioTrackPin->AudioStreamR = CreateInputPin<UM2MetasoundLiteralPin>(Handle);
						}

						break;
				}

				continue;
			}
		
		}

		if(InputM2SoundPins.Contains(PinName))
		{
			PinObject = InputM2SoundPins[PinName];
			PinObject->SetHandle(Handle);
			PinObject->bIsStale = false;
		}
		else
		{
			PinObject = CreateInputPin<UM2MetasoundLiteralPin>(Handle);
			InputM2SoundPins.Add(PinName, PinObject);
		}

	}


	//now the outputs

	for (const auto& Handle : OutHandles)
	{
		
		FName PinName;
		FName DataType;
		FName SearchName;
		BuilderContext.GetNodeOutputData(Handle, PinName, DataType, BuildResult);
		//auto LiteralValue = BuilderContext.GetNodeOutputClassDefault(Handle, BuildResult);

		UM2Pins* PinObject = nullptr;

		using namespace M2Sound::Pins;
		{
			if (PinCategoryMap.Contains(PinName))
			{
				auto PinCategory = PinCategoryMap[PinName];
				SearchName = AutoDiscovery::AudioTrack;
				if (OutputM2SoundPins.Contains(SearchName))
				{
					PinObject = OutputM2SoundPins[SearchName];
					PinObject->bIsStale = false;
				}
				else
				{
					PinObject = CreateAudioTrackOutputPin();
					OutputM2SoundPins.Add(SearchName, PinObject);
				}

				auto* AsAudioTrackPin = Cast<UM2AudioTrackPin>(PinObject);

				switch (PinCategory)
				{
				case EVertexAutoConnectionPinCategory::AudioStreamL:
					if (IsValid(AsAudioTrackPin->AudioStreamL))
					{
						AsAudioTrackPin->AudioStreamL->SetHandle(Handle);
					}
					else
					{
						AsAudioTrackPin->AudioStreamL = CreateOutputPin<UM2MetasoundLiteralPin>(Handle);
					}
					break;
				case EVertexAutoConnectionPinCategory::AudioStreamR:
					if (IsValid(AsAudioTrackPin->AudioStreamR))
					{
						AsAudioTrackPin->AudioStreamR->SetHandle(Handle);
					}
					else
					{
						AsAudioTrackPin->AudioStreamR = CreateOutputPin<UM2MetasoundLiteralPin>(Handle);
					}

					break;
				}

				continue;
			}

		}

		if (OutputM2SoundPins.Contains(PinName))
		{
			PinObject = OutputM2SoundPins[PinName];
			PinObject->SetHandle(Handle);
			PinObject->bIsStale = false;
		}
		else
		{
			PinObject = CreateOutputPin<UM2MetasoundLiteralPin>(Handle);
			OutputM2SoundPins.Add(PinName, PinObject);
		}
	}


	RemoveAllStalePins();
}


void UM2SoundVertex::UpdateValueForPin(FM2SoundPinData& Pin, FMetasoundFrontendLiteral& NewValue)
{
	//in theory we should should check whether this pin has a param connected... but we're just going to set the value for now
	
	EMetaSoundBuilderResult BuildResult;
	GetSequencerData()->BuilderContext->SetNodeInputDefault(Pin.InputHandle, NewValue, BuildResult);

	if(BuildResult == EMetaSoundBuilderResult::Failed)	BuilderConnectionResults.Add(Pin.PinName, BuildResult);
}


UDAWSequencerData* UM2SoundVertex::GetSequencerData() const
{
	return SequencerData;
}

UMetaSoundSourceBuilder& UM2SoundVertex::GetBuilderContext() const
{
	return *SequencerData->BuilderContext;
	
	// TODO: insert return statement here
}

void UM2SoundVertex::VertexNeedsBuilderUpdates()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Vertex needs builder updates!"))
	BuildVertex();
	CollectParamsForAutoConnect();
	UpdateConnections();

	//need to inform downstream vertexes to reconnect to our output

	OnVertexUpdated.Broadcast();
	//OnVertexNeedsBuilderConnectionUpdates.Broadcast(this);
}

void UM2SoundVertex::VertexConnectionsChanged()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Vertex connections changed!"))
	UpdateConnections();
	//OnVertexNeedsBuilderConnectionUpdates.Broadcast(this);
}

void UM2SoundVertex::TransmitAudioParameter(FAudioParameter Parameter)
{
	if (GetSequencerData())
	{
		GetSequencerData()->ReceiveAudioParameter(Parameter);
	}
	else {
		UE_LOG(unDAWVertexLogs, Error, TEXT("Outer is not sequencer data FFS!"))
	}
}

inline bool ResultToBool(EMetaSoundBuilderResult& Result)
{
	return Result == EMetaSoundBuilderResult::Succeeded;
}

#define TOFLAG(Enum) (1 << static_cast<uint8>(Enum))

void UM2SoundVertex::CollectParamsForAutoConnect()
{

	
	EMetaSoundBuilderResult BuildResult;

	auto& BuilderSubsystems = SequencerData->MSBuilderSystem;
	auto& BuilderContext = SequencerData->BuilderContext;

	TryFindVertexDefaultRangesInCache();

	//find the inputs and outputs of the node
	//InPins = BuilderContext->FindNodeInputs(NodeHandle, BuildResult);
	for (const auto& Input : InPins)
	{



		FM2SoundPinData PinData;

		FName PinName;
		FName DataType;
		PinData.InputHandle = Input;
		BuilderContext->GetNodeInputData(Input, PinName, DataType, BuildResult);
		auto LiteralValue = BuilderContext->GetNodeInputClassDefault(Input, BuildResult);

	}
	OnVertexUpdated.Broadcast();

}

#if WITH_EDITOR

void UM2SoundVertex::UpdateConnections()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Updating Connections"))
	for (const auto& [Name, Pin] : InputM2SoundPins)
	{
		if(Pin->LinkedPin)
		{
			bool result;
			if (auto* AsAudioTrack = Cast<UM2AudioTrackPin>(Pin))
			{
				result = GetSequencerData()->ConnectPins<UM2AudioTrackPin>(AsAudioTrack, Cast<UM2AudioTrackPin>(Pin->LinkedPin));
			}
			else {
				result = GetSequencerData()->ConnectPins<UM2MetasoundLiteralPin>(Cast<UM2MetasoundLiteralPin>(Pin), Cast<UM2MetasoundLiteralPin>(Pin->LinkedPin));
			}

			Pin->ConnectionResult = result ? EMetaSoundBuilderResult::Succeeded : EMetaSoundBuilderResult::Failed;
			
		}
	}
}

void UM2SoundVertex::DestroyVertexInternal()
{
	

	//if(MainInput)
	//{
	//	MainInput->UnregisterOutputVertex(this);
	//}

	DestroyVertex();
}

void UM2SoundVertex::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	//VertexNeedsBuilderUpdates();
	//let's just do a quick test, print param name
	//UE_LOG(unDAWVertexLogs, Verbose, TEXT("PostEditChangeProperty %s"), *GetName())
	auto Property = PropertyChangedEvent.Property;
	auto PropertyName = Property->GetFName();

	if (PropertyName == FName(TEXT("DisplayFlags")))
	{
		OnVertexUpdated.Broadcast();
	}

	if (PropertyName == FName(TEXT("MinValue")))
	{
		OnVertexUpdated.Broadcast();
	}	
	
	if (PropertyName == FName(TEXT("MaxValue")))
	{
		OnVertexUpdated.Broadcast();
	}
	//auto PropertyValue = (uint8*) Property->CallGetter(this);

}


#endif

#undef TOFLAG


void UM2SoundAudioOutput::BuildVertex()
{
	auto& BuilderSubsystems = SequencerData->MSBuilderSystem;
	auto& BuilderContext = SequencerData->BuilderContext;
	BuilderResults.Empty();

	//EMetaSoundBuilderResult BuildResult;
	AudioOutput = FAssignableAudioOutput();
	AudioOutput.AudioLeftOutputInputHandle = SequencerData->CoreNodes.AudioOuts[0];
	AudioOutput.AudioRightOutputInputHandle = SequencerData->CoreNodes.AudioOuts[1];
	AudioOutput.OutputName = FName(TEXT("Master Output"));

		//SequencerData->CoreNodes.GetFreeMasterMixerAudioOutput();
	BuilderResults.Add(FName(TEXT("Assigned Output")), EMetaSoundBuilderResult::Succeeded);
	GainParameterName = AudioOutput.OutputName;
	TArray<FMetaSoundBuilderNodeInputHandle> MappedInputs;
	MappedInputs.Add(AudioOutput.AudioLeftOutputInputHandle);
	MappedInputs.Add(AudioOutput.AudioRightOutputInputHandle);
	PopulatePinsFromMetasoundData(MappedInputs, {});

	
	//auto* InPin = CreateAudioTrackInputPin();
	//InPin->AudioStreamL = CreateInputPin<UM2MetasoundLiteralPin>(AudioOutput.AudioLeftOutputInputHandle);
	//InPin->AudioStreamR = CreateInputPin<UM2MetasoundLiteralPin>(AudioOutput.AudioRightOutputInputHandle);
	//InputM2SoundPins.Add(M2Sound::Pins::AutoDiscovery::AudioTrack, InPin);
	

	//FName OutDataType;
	//auto NewGainInput = BuilderContext->AddGraphInputNode(GainParameterName, TEXT("float"), BuilderSubsystems->CreateFloatMetaSoundLiteral(1.f, OutDataType), BuildResult);
	//OutPins.Add(NewGainInput);
	//BuilderContext->ConnectNodes(NewGainInput, AudioOutput.GainParameterInputHandle, BuildResult);
	//BuilderResults.Add(FName(TEXT("Assigned Gain Param")), BuildResult);

}

//void UM2SoundAudioOutput::UpdateConnections()
//{
	//return;
	//
	////if no main input, do nothing
	//BuilderConnectionResults.Empty();
	//if (!MainInput)
	//{
	//	//need to disconnect our own inputs
	//	//auto& BuilderSubsystems = SequencerData->MSBuilderSystem;
	//	auto& BuilderContext = SequencerData->BuilderContext;

	//	if(!BuilderContext)
	//	{
	//		UE_LOG(unDAWVertexLogs, VeryVerbose, TEXT("Builder Context is null!"))
	//		return;
	//	}

	//	EMetaSoundBuilderResult BuildResult;
	//	BuilderContext->DisconnectNodeInput(AudioOutput.AudioLeftOutputInputHandle, BuildResult);
	//	BuilderConnectionResults.Add(FName(TEXT("Disconnect Audio Stream L")), BuildResult);
	//	BuilderContext->DisconnectNodeInput(AudioOutput.AudioRightOutputInputHandle, BuildResult);
	//	BuilderConnectionResults.Add(FName(TEXT("Disconnect Audio Stream R")), BuildResult);
	//			
	//	UE_LOG(unDAWVertexLogs, VeryVerbose, TEXT("Main Input is null!"))
	//	return;
	//}

	////otherwise, well, we have an AssignedOutput, we need to find the audio streams of the upstream vertex, connect them and that's it, show me what you got co-pilot

	//auto& BuilderSubsystems = SequencerData->MSBuilderSystem;
	//auto& BuilderContext = SequencerData->BuilderContext;
	//EMetaSoundBuilderResult BuildResult;

	////find the audio stream outputs of the main input using the AutoConnectOutPins
	//auto UpStreamLeftAudio = MainInput->AutoConnectOutPins.Find(EVertexAutoConnectionPinCategory::AudioStreamL);
	//auto UpStreamRightAudio = MainInput->AutoConnectOutPins.Find(EVertexAutoConnectionPinCategory::AudioStreamR);

	////connect the audio streams to the audio output node
	//bool Success = false;
	//if (UpStreamLeftAudio)
	//{
	//	BuilderContext->ConnectNodes(*UpStreamLeftAudio, AudioOutput.AudioLeftOutputInputHandle, BuildResult);
	//	BuilderResults.Add(FName(TEXT("Connect Audio Stream L to Audio Output")), BuildResult);
	//	Success = ResultToBool(BuildResult);
	//}

	//if (UpStreamRightAudio)
	//{
	//	BuilderContext->ConnectNodes(*UpStreamRightAudio, AudioOutput.AudioRightOutputInputHandle, BuildResult);
	//	BuilderResults.Add(FName(TEXT("Connect Audio Stream R to Audio Output")), BuildResult);
	//	Success = ResultToBool(BuildResult) && Success;
	//}

	//if(Success)
	//{
	//	BuilderConnectionResults.Add(FName(TEXT("Audio Output Connection Success")), EMetaSoundBuilderResult::Succeeded);
	//}
	//else
	//{
	//	BuilderConnectionResults.Add(FName(TEXT("Audio Output Connection Failed")), EMetaSoundBuilderResult::Failed);
	//}

//
//}

void UM2SoundAudioOutput::DestroyVertex()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Destroying Audio Output Vertex"))
	GetSequencerData()->CoreNodes.ReleaseMasterMixerAudioOutput(AudioOutput);

	GetSequencerData()->RemoveVertex(this);
}

void UM2SoundAudioOutput::CollectAndTransmitAudioParameters()
{
	//this is a test, we just need to transmit the gain parameter
	TransmitAudioParameter(FAudioParameter(AudioOutput.OutputName, Gain));
}


void UM2SoundBuilderInputHandleNode::BuildVertex()
{


	TArray<FMetaSoundBuilderNodeOutputHandle> MappedOutput;
	MappedOutput.Add(SequencerData->CoreNodes.MappedOutputs[TrackId].OutputHandle);

	TArray<FMetaSoundBuilderNodeInputHandle> MappedInputs;
	PopulatePinsFromMetasoundData(MappedInputs, MappedOutput);
	//OutputM2SoundPins.Empty();

	//UM2MetasoundLiteralPin* NewOutputPin = CreateOutputPin<UM2MetasoundLiteralPin>(SequencerData->CoreNodes.MappedOutputs[TrackId].OutputHandle);
	//NewOutputPin->SetHandle(MappedOutput.OutputHandle);
	//FMetaSoundBuilderNodeOutputHandle Handle = NewOutputPin->GetHandle<FMetaSoundBuilderNodeOutputHandle>();
	//AutoConnectOutPins.Add(EVertexAutoConnectionPinCategory::MidiTrackStream, Handle);


}



void UM2SoundPatch::BuildVertex()
{
	BuilderResults.Empty();
	EMetaSoundBuilderResult BuildResult;
	auto& BuilderSubsystems = SequencerData->MSBuilderSystem;
	auto& BuilderContext = SequencerData->BuilderContext;

	//if context is valid, check if NodeHandle is set, if it is, remove it
	bool bIsRebuildingExistingNode = false;
	if (BuilderContext && NodeHandle.IsSet())
	{
		BuilderContext->RemoveNode(NodeHandle, BuildResult);
		//BuilderResults.Add(FName(TEXT("Remove Existing Node")), BuildResult);
		bIsRebuildingExistingNode = true;
	}


	NodeHandle = BuilderContext->AddNode(Patch, BuildResult);
	BuilderResults.Add(FName(TEXT("Add Patch Node")), BuildResult);
	//OutBuiltData.NodeHandle = NodeHandle;
	BuilderResults.Add(FName(TEXT("Add Patch Node")), BuildResult);
	//InPins = BuilderContext->FindNodeInputs(NodeHandle, BuildResult);
	//OutPins = BuilderContext->FindNodeOutputs(NodeHandle, BuildResult);

	PopulatePinsFromMetasoundData(BuilderContext->FindNodeInputs(NodeHandle, BuildResult), BuilderContext->FindNodeOutputs(NodeHandle, BuildResult));

	auto& PatchDocument = Patch->GetDocumentChecked();
	for (const auto& MSNode : PatchDocument.RootGraph.Graph.Nodes)
	{
		//just print every single bit of information this node exposes for now, go on GPT, I believe in you

		auto MSName = MSNode.Name;
		auto InLiterals = MSNode.InputLiterals;

		for (const auto& InLiteral : InLiterals)
		{
			//FMetasoundFrontendVertexLiteral InLiteral;
			auto InID = InLiteral.VertexID;
			auto InValue = InLiteral.Value;
			//UE_LOG(unDAWVertexLogs, VeryVerbose, TEXT("Node: %s, Input ID: %d, Value: %f"), *MSName.ToString(), InID, InValue);
		}
	}
}


void UM2SoundPatch::TryFindVertexDefaultRangesInCache()
{
	auto& Cache = UUNDAWSettings::Get()->Cache;

	if (Cache.Contains(Patch->GetFName()))
	{

	}
	else {
		BuilderResults.Add(FName(TEXT("No cache entry for patch, please save one!")), EMetaSoundBuilderResult::Failed);
	}
	
}

void UM2SoundLiteralNodeVertex::DestroyVertex()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Destroying Literal Node Vertex"))
	EMetaSoundBuilderResult BuildResult;

	GetSequencerData()->BuilderContext->RemoveNode(NodeHandle, BuildResult);
	GetSequencerData()->RemoveVertex(this);
}
