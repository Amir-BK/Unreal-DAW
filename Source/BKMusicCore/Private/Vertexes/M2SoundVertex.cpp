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

FLinearColor UM2SoundVertex::GetVertexColor() const
{
	// should override in subclasses
	if(ColorSourcePin && ColorSourcePin->LinkedPin)
	{
		return ColorSourcePin->LinkedPin->ParentVertex->GetVertexColor();
	}
	return FLinearColor::Blue;
}

void UM2SoundVertex::PopulatePinsFromMetasoundData(const TArray<FMetaSoundBuilderNodeInputHandle>& InHandles, const TArray<FMetaSoundBuilderNodeOutputHandle>& OutHandles)
{
	EMetaSoundBuilderResult BuildResult;
	UMetaSoundSourceBuilder& BuilderContext = GetBuilderContext();
	MarkAllPinsStale();

	for (const auto& Handle : InHandles)
	{
		FName PinName;
		FName DataType;
		FName SearchName;
		BuilderContext.GetNodeInputData(Handle, PinName, DataType, BuildResult);
		auto LiteralValue = BuilderContext.GetNodeInputClassDefault(Handle, BuildResult);

		bool bIsConstructorPin = BuilderContext.GetNodeInputIsConstructorPin(Handle);

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

				switch (PinCategory)
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


				if(ColorSourcePin == nullptr)
				{
					ColorSourcePin = AsAudioTrackPin;
				}

				continue;
			}
		}

		if (InputM2SoundPins.Contains(PinName))
		{
			PinObject = InputM2SoundPins[PinName];
			PinObject->SetHandle(Handle);
			PinObject->bIsStale = false;
		}
		else
		{
			auto PinLiteralObject = CreateInputPin<UM2MetasoundLiteralPin>(Handle);
			PinLiteralObject->bIsConstructorPin = bIsConstructorPin;
			if(!ColorSourcePin && DataType == FName("MIDIStream"))
			{
				ColorSourcePin = PinLiteralObject;
			}

			//if Pin is a trigger or a timestamp we need to create a graph input
			if (PinLiteralObject->DataType == FName("Trigger") || PinLiteralObject->DataType == FName("MusicTimeStamp"))
			{
				auto VertexName = GetName();
				auto PinNameString = PinName.ToString();
				PinLiteralObject->AutoGeneratedGraphInputName = FName(FString::Printf(TEXT("%s.%s"), *VertexName, *PinNameString));
			}

			PinLiteralObject->LiteralValue = LiteralValue;

			InputM2SoundPins.Add(PinName, PinLiteralObject);
		}

		//PinObject->bIsConstructorPin = bIsConstructorPin;
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

	if (BuildResult == EMetaSoundBuilderResult::Failed)	BuilderConnectionResults.Add(Pin.PinName, BuildResult);
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
	//CollectParamsForAutoConnect();
	UpdateConnections();

	//need to inform downstream vertexes to reconnect to our output

	OnVertexUpdated.Broadcast();
	//OnVertexNeedsBuilderConnectionUpdates.Broadcast(this);
}

void UM2SoundVertex::VertexConnectionsChanged()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Vertex connections changed!"))
	UpdateConnections();

	bool bBuildCyclicalDownstreamTree = true;
	if (bBuildCyclicalDownstreamTree)
	{
		//need to inform downstream vertexes to reconnect to our output
		UE_LOG(unDAWVertexLogs, Verbose, TEXT("Building cyclical downstream tree"))
			//OnVertexUpdated.Broadcast();
			//OnVertexNeedsBuilderConnectionUpdates.Broadcast(this);
		//OnVertexUpdated.Broadcast();
	}
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

	checkNoEntry();

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

void UM2SoundVertex::UpdateConnections()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Updating Connections"))
		for (const auto& [Name, Pin] : InputM2SoundPins)
		{
			auto AsLiteral = Cast<UM2MetasoundLiteralPin>(Pin);
			if (AsLiteral)
			{
				EMetaSoundBuilderResult BuildResult;
				GetSequencerData()->BuilderContext->SetNodeInputDefault(AsLiteral->GetHandle<FMetaSoundBuilderNodeInputHandle>(), AsLiteral->LiteralValue, BuildResult);
			}
			
			if (Pin->LinkedPin)
			{
				bool result;
				if (auto* AsAudioTrack = Cast<UM2AudioTrackPin>(Pin))
				{
					result = GetSequencerData()->ConnectPins<UM2AudioTrackPin>(AsAudioTrack, Cast<UM2AudioTrackPin>(Pin->LinkedPin));
				}
				else {
					result = GetSequencerData()->ConnectPins<UM2MetasoundLiteralPin>(Cast<UM2MetasoundLiteralPin>(Pin), Cast<UM2MetasoundLiteralPin>(Pin->LinkedPin));
				}

				//auto AsLiteral = Cast<UM2MetasoundLiteralPin>(Pin);
				if (AsLiteral && AsLiteral->DataType == FName("Trigger"))
				{
					EMetaSoundBuilderResult BuildResult;
					auto GraphInput = GetBuilderContext().AddGraphInputNode(AsLiteral->AutoGeneratedGraphInputName, AsLiteral->DataType, FMetasoundFrontendLiteral(), BuildResult);
					//GetBuilderContext().ConnectNodes(GraphInput, AsLiteral->GetHandle<FMetaSoundBuilderNodeInputHandle>(), BuildResult);
				}

				Pin->ConnectionResult = result ? EMetaSoundBuilderResult::Succeeded : EMetaSoundBuilderResult::Failed;
			}
			else {
				// if pin is a trigger literal and it's not connected to anything for now we will connect it to a graph input
				
				if(AsLiteral && AsLiteral->DataType == FName("Trigger"))
				{
					EMetaSoundBuilderResult BuildResult;
					//GetSequencerData()->BuilderContext->AddGraphInput(AsLiteral->AutoGeneratedGraphInputName, AsLiteral->DataType, AsLiteral->DefaultValue, AsLiteral->InputHandle);
					auto GraphInput = GetBuilderContext().AddGraphInputNode(AsLiteral->AutoGeneratedGraphInputName, AsLiteral->DataType, FMetasoundFrontendLiteral(), BuildResult);
					//connect the literal to the graph input
					GetBuilderContext().ConnectNodes(GraphInput, AsLiteral->GetHandle<FMetaSoundBuilderNodeInputHandle>(), BuildResult);

				}

				if(AsLiteral && AsLiteral->DataType == FName("MusicTimeStamp"))
				{
					EMetaSoundBuilderResult BuildResult;
					auto GraphInput = GetBuilderContext().AddGraphInputNode(AsLiteral->AutoGeneratedGraphInputName, AsLiteral->DataType, FMetasoundFrontendLiteral(), BuildResult);

					GetBuilderContext().ConnectNodes(GraphInput, AsLiteral->GetHandle<FMetaSoundBuilderNodeInputHandle>(), BuildResult);
					SequencerData->StructParametersPack->SetParameter(AsLiteral->AutoGeneratedGraphInputName, FName("MusicTimeStamp"), AsLiteral->Timestamp, false);
					SequencerData->ApplyParameterPack();
					//GetBuilderContext().ConnectNodes(GraphInput, AsLiteral->GetHandle<FMetaSoundBuilderNodeInputHandle>(), BuildResult);
				}
				
			}
		}

	

	if (!bIsRebuilding) return;

	for (const auto& [Name, Pin] : OutputM2SoundPins)
	{
		if (Pin->LinkedPin)
		{
			bool result;
			if (auto* AsAudioTrack = Cast<UM2AudioTrackPin>(Pin))
			{
				result = GetSequencerData()->ConnectPins<UM2AudioTrackPin>(Cast<UM2AudioTrackPin>(Pin->LinkedPin), AsAudioTrack);
			}
			else {
				result = GetSequencerData()->ConnectPins<UM2MetasoundLiteralPin>(Cast<UM2MetasoundLiteralPin>(Pin->LinkedPin), Cast<UM2MetasoundLiteralPin>(Pin));
			}

			Pin->ConnectionResult = result ? EMetaSoundBuilderResult::Succeeded : EMetaSoundBuilderResult::Failed;
		}
	}

	bIsRebuilding = false;
}

void UM2SoundVertex::DestroyVertexInternal()
{
	//if(MainInput)
	//{
	//	MainInput->UnregisterOutputVertex(this);
	//}

	DestroyVertex();
}

inline void UM2SoundVertex::MarkAllPinsStale() {
	for (auto& Pin : InputM2SoundPins)
	{
		Pin.Value->bIsStale = true;
	}

	for (auto& Pin : OutputM2SoundPins)
	{
		Pin.Value->bIsStale = true;
	}
}

inline void UM2SoundVertex::RemoveAllStalePins()
{
	TArray<FName> StalePins;

	for (auto& Pin : InputM2SoundPins)
	{
		if (Pin.Value->bIsStale)
		{
			StalePins.Add(Pin.Key);
		}
	}

	for (auto& Pin : StalePins)
	{
		InputM2SoundPins.Remove(Pin);
	}

	StalePins.Empty();

	for (auto& Pin : OutputM2SoundPins)
	{
		if (Pin.Value->bIsStale)
		{
			StalePins.Add(Pin.Key);
		}
	}

	for (auto& Pin : StalePins)
	{
		OutputM2SoundPins.Remove(Pin);
	}
}

#if WITH_EDITOR

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
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Building Audio Output Vertex"))

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
}

void UM2SoundAudioOutput::DestroyVertex()
{
}

void UM2SoundAudioOutput::CollectAndTransmitAudioParameters()
{
	//this is a test, we just need to transmit the gain parameter
	TransmitAudioParameter(FAudioParameter(AudioOutput.OutputName, Gain));
}

void UM2SoundBuilderInputHandleVertex::BuildVertex()
{
	TArray<FMetaSoundBuilderNodeOutputHandle> MappedOutput;
	//auto MidiTrackName = SequencerData->GetTracksDisplayOptions(TrackId).trackName + ".MidiStream";
	MappedOutput.Add(SequencerData->CoreNodes.MemberInputMap[FName(MemberName)].MemberInputOutputHandle);

	TArray<FMetaSoundBuilderNodeInputHandle> MappedInputs;
	PopulatePinsFromMetasoundData(MappedInputs, MappedOutput);
}

FLinearColor UM2SoundBuilderInputHandleVertex::GetVertexColor() const
{
	return SequencerData->GetTrackMetadata(TrackId).trackColor;
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
		bIsRebuilding = true;
	}

	if (!IsValid(Patch))
	{
		BuilderResults.Add(FName(TEXT("No Patch Assigned or patch is invalid")), EMetaSoundBuilderResult::Failed);
		return;
	
	}

	NodeHandle = BuilderContext->AddNode(Patch, BuildResult);
	BuilderResults.Add(FName(TEXT("Add Patch Node")), BuildResult);
	//OutBuiltData.NodeHandle = NodeHandle;
	//BuilderResults.Add(FName(TEXT("Add Patch Node")), BuildResult);
	//InPins = BuilderContext->FindNodeInputs(NodeHandle, BuildResult);
	//OutPins = BuilderContext->FindNodeOutputs(NodeHandle, BuildResult);

	PopulatePinsFromMetasoundData(BuilderContext->FindNodeInputs(NodeHandle, BuildResult), BuilderContext->FindNodeOutputs(NodeHandle, BuildResult));

}

void UM2SoundPatch::TryFindVertexDefaultRangesInCache()
{
	auto& Cache = UUNDAWSettings::Get()->Cache;
	BuilderResults.Empty();


	if (Cache.Contains(Patch->GetFName()))
	{
	}
	else {
		//BuilderResults.Add(FName(TEXT("No cache entry for patch, please save one!")), EMetaSoundBuilderResult::Failed);
	}
}

void UM2SoundLiteralNodeVertex::DestroyVertex()
{
	UE_LOG(unDAWVertexLogs, Verbose, TEXT("Destroying Literal Node Vertex"))
		EMetaSoundBuilderResult BuildResult;

	GetSequencerData()->BuilderContext->RemoveNode(NodeHandle, BuildResult);
	GetSequencerData()->RemoveVertex(this);
}

void UM2SoundDynamicGraphInputVertex::RenameInput(FName InMemberName)
{
	if (GetSequencerData()->RenameNamedInput(MemberName, InMemberName))
	{
		MemberName = InMemberName;
		OutputM2SoundPins[FName("GraphInput")]->AutoGeneratedGraphInputName = MemberName;
	}
}

void UM2SoundDynamicGraphInputVertex::BuildVertex()
{
	////we need to check that the name is not already used and that we are connected to something
	EMetaSoundBuilderResult BuildResult;
	//auto& BuilderSubsystems = SequencerData->MSBuilderSystem;
	//auto& BuilderContext = SequencerData->BuilderContext;

	//if (OutputM2SoundPins.IsEmpty())
	//{
	//	//This is fine, we're uninitialized 
	//}
	if(OutputM2SoundPins.IsEmpty())
	{
	auto* NewPin = CreateWildCardOutPin();
	NewPin->AutoGeneratedGraphInputName = MemberName;
	OutputM2SoundPins.Add(FName("GraphInput"), NewPin);
	GetSequencerData()->NamedInputs.Add(MemberName, NewPin);
	NewPin->bIsSet = false;
	bIsSet = false;
	}else
	{
		auto LinkedPinAsLiteralPin = Cast<UM2MetasoundLiteralPin>(OutputM2SoundPins[FName("GraphInput")]->LinkedPin);
		if (!LinkedPinAsLiteralPin) return;
		auto LinkedPinDataTypeName = LinkedPinAsLiteralPin->DataType;
		GetBuilderContext().AddGraphInputNode(MemberName, LinkedPinDataTypeName, FMetasoundFrontendLiteral(),  BuildResult);
	}

}
