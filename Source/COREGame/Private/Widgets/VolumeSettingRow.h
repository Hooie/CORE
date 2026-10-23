// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/CAudioSettings.h"
#include "VolumeSettingRow.generated.h"

/**
 * 
 */
UCLASS()
class UVolumeSettingRow : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY(EditAnywhere, Category = "Volume")
	ESoundVolumeCategory Category = ESoundVolumeCategory::Master;

	UPROPERTY(EditAnywhere, Category = "Volume")
	FText DisplayName;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* NameText;

	UPROPERTY(meta = (BindWidget))
	class USlider* VolumeSlider;

	UPROPERTY(meta = (BindWidget))
	class UEditableText* ValueText;

	UFUNCTION()
	void OnSliderValueChanged(float Value);

	UFUNCTION()
	void OnSliderReleased();

	UFUNCTION()
	void OnValueTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void UpdateValueText(float Value);

};
