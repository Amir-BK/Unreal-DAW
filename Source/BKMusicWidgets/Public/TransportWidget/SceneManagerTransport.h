// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Components/Slider.h"
#include "BK_MusicSceneManagerInterface.h"
#include "GlyphButton.h"
#include "SceneManagerTransport.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeekValueChangedEvent, float, Value);

//macro to make declaring the transport actions easier
#define TRANSPORTACTION(Button)		if (Button) { Button->TransportButtonClicked.AddUniqueDynamic(this, &USceneManagerTransportWidget::ReceiveButtonClick);}
/**
 * Base class for the scene manager transport, you have to inherit this in UMG and use the button binds to make it work. See the BP example.
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class BKMUSICWIDGETS_API USceneManagerTransportWidget : public UUserWidget
{
	GENERATED_BODY()

public: 
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "unDAW|Transport")
	FOnTransportCommand TransportCalled;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "unDAW|Transport")
	FOnSeekValueChangedEvent TransportSeekCommand;

	UPROPERTY(BlueprintReadOnly, Category = "unDAW|Transport")
	TEnumAsByte<EBKPlayState> TransportPlayState;

	UFUNCTION()
	void ReceiveButtonClick(EBKTransportCommands newCommand)
	{
		TransportCalled.Broadcast(newCommand);
	}


protected:



	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USlider* PlayPosition;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTransportGlyphButton* PlayButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTransportGlyphButton* PauseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTransportGlyphButton* InitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTransportGlyphButton* StopButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTransportGlyphButton* KillButton;

	UPROPERTY()
	float CurrentSeek = 0;

public:

	UFUNCTION(BlueprintCallable, Category = "unDAW|Transport")
	void SetTransportDuration(float newDuration)
	{
		if (PlayPosition) PlayPosition->SetMaxValue(newDuration);
	}

	UFUNCTION(BlueprintCallable, Category = "unDAW|Transport")
	void SetTransportSeek(float NewSeek)
	{
		
		if (NewSeek != CurrentSeek)
		{
			if (PlayPosition) PlayPosition->SetValue(NewSeek);
			CurrentSeek = NewSeek;
			TransportSeekCommand.Broadcast(NewSeek);
		}

	}

	UFUNCTION(BlueprintCallable, Category = "unDAW|Transport")
	void SetTransportPlayState(EBKPlayState newPlayState)
	{
		TransportPlayState = newPlayState;
	}

	virtual void NativeConstruct() override
	{
		Super::NativeConstruct();

		bIsVariable = true;

		TRANSPORTACTION(PauseButton)
		TRANSPORTACTION(InitButton)
		TRANSPORTACTION(StopButton)
		TRANSPORTACTION(KillButton)
		TRANSPORTACTION(PlayButton)

		if (PlayPosition)
			{
			PlayPosition->OnValueChanged.AddUniqueDynamic(this, &USceneManagerTransportWidget::SetTransportSeek);
			}
	}
};

#undef TRANSPORTACTION