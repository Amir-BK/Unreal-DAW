// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundGeneratorHandle.h"
#include "GameFramework/Actor.h"
#include "BK_MusicSceneManagerInterface.h"
#include "Components/AudioComponent.h"
#include "MusicScenePlayerActor.generated.h"

UCLASS()
class BKMUSICCORE_API AMusicScenePlayerActor : public AActor, public IBK_MusicSceneManagerInterface
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

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "unDAW|Music Scene")
	TObjectPtr<UAudioComponent> Audio;


	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "unDAW")
	UAudioComponent* AudioComponent;

	UPROPERTY()
	UMetasoundGeneratorHandle* GeneratorHandle;	

	UPROPERTY()
	TEnumAsByte<EBKPlayState> PlayState = EBKPlayState::NotReady;

	UPROPERTY()
	float PlaybackCursorPosition = 0.0f;

	//METHODS

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// TODO [$65cfdef41013620009101dda]: implement clock init in MusicSceneActor, register to clock events and route them to subscribers
	UFUNCTION(BlueprintCallable, Category = "BK Music")
	virtual void InitClock(float inBPM);

	UFUNCTION(BlueprintCallable, Category = "BK Music")
	void UpdateWatchers();

	FOnTransportSeekCommand* GetSeekCommandDelegate() override;

	FOnPlaybackStateChanged* GetPlaybackStateDelegate() override;



	UFUNCTION()
	void SendTransportCommand(EBKTransportCommands InCommand) override;

	UFUNCTION()
	void SendSeekCommand(float InSeek) override;

	UFUNCTION(BlueprintImplementableEvent, CallInEditor, Category = "BK Music")
	void ReceivedSeekUpdate(float InSeek);

	// Inherited via IBK_MusicSceneManagerInterface
	UAudioComponent* GetAudioComponent() override;

	// Inherited via IBK_MusicSceneManagerInterface
	const EBKPlayState GetCurrentPlaybackState() override;
};
