// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundGeneratorHandle.h"
#include "GameFramework/Actor.h"
#include "BK_MusicSceneManagerInterface.h"
#include "Components/AudioComponent.h"
#include "HarmonixMetasound/Components/MusicClockComponent.h"
#include "HarmonixMetasound/Components/MusicTempometerComponent.h"
#include "HarmonixMidi/MidiSongPos.h"

#include "Delegates/DelegateBase.h"
#include "Delegates/DelegateSettings.h"
#include "MusicScenePlayerActor.generated.h"


UCLASS()
class BKMUSICCORE_API AMusicScenePlayerActor : public AActor //, public IBK_MusicSceneManagerInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMusicScenePlayerActor();

	// DELEGATES!!!

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "unDAW|Transport")
	FOnTransportSeekCommand TransportSeekDelegate;

	UPROPERTY(BlueprintAssignable, Category = "unDAW|Transport")
	FOnPlaybackStateChanged PlaystateDelegate;


	//PROPERTIES



	UPROPERTY()
	UDAWSequencerPerformer* BuilderHelper;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "unDAW|Music Scene")
	UAudioComponent* PerformanceAudioComponent;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "unDAW|Music Scene")
	UMusicClockComponent* VideoSyncedMidiClock;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "unDAW|Music Scene")
	UMusicTempometerComponent* MusicTempometer;



	//UPROPERTY()
	TSharedPtr<UMetasoundGeneratorHandle> GeneratorHandle;

	UPROPERTY()
	TEnumAsByte<EBKPlayState> PlayState = EBKPlayState::NotReady;

	UPROPERTY()
	float PlaybackCursorPosition = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "unDAW", meta = (DisplayPriority = "0"))
	TObjectPtr<UDAWSequencerData> SessionData;

	//METHODS

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void PerformanceMetasoundGeneratorCreated(TSharedPtr<Metasound::FMetasoundGenerator> GeneratorPointer, ESPMode UserPolicy);

	void PerformanceMetasoundGeneratorDestroyed(uint64 GeneratorPointer);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// TODO [$65cfdef41013620009101dda]: implement clock init in MusicSceneActor, register to clock events and route them to subscribers
	UFUNCTION(BlueprintCallable, Category = "BK Music")
	virtual void InitClock(float inBPM);

	UFUNCTION(BlueprintCallable, Category = "BK Music")
	void UpdateWatchers();

};
