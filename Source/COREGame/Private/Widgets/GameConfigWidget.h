// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameConfigWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnReturnBtnClicked);

class UButton;
class USlider;
class UTextBlock;
class UComboBoxString;
class UEditableText;

/**
 * 
 */
UCLASS()
class UGameConfigWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FOnReturnBtnClicked OnReturnBtnClicked;

	virtual void NativeConstruct() override;
	void InitializeSettings();

private:
	class UCGameUserSettings* Settings;

	UPROPERTY(meta=(BindWidget))
	UButton* ReturnBtn;

	UFUNCTION()
	void ReturnBtnClicked();

	/*************************************/
	/***********Graphic Setting***********/
	/*************************************/
	UPROPERTY(meta=(BindWidget))
	UComboBoxString* WindowModeComboBox;

	UFUNCTION()
	void WindowModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* ResolutionComboBox;

	UFUNCTION()
	void ResolutionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void UpdateResolutionComboBoxEnabled();

	void SyncResolutionComboBoxToCurrentResolution();

	TOptional<FIntPoint> CachedWindowResolution;

	UPROPERTY(meta=(BindWidget))
	UButton* QualityLeftArrowBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* QualityRightArrowBtn;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* QualityValueText;

	UPROPERTY(meta = (BindWidget))
	UButton* QualityApplyBtn;

	UFUNCTION()
	void QualityLeftArrowBtnClicked();

	UFUNCTION()
	void QualityRightArrowBtnClicked();

	UFUNCTION()
	void QualityApplyBtnClicked();

	int PendingQualityLevel = 2;

	void ChangePendingQualityLevel(int Delta);
	void RefreshQualityDisplay();
	FString GetQualityLevelLabel(int Level) const;

	void ApplyMouseLockMode(EWindowMode::Type Mode);

	/*************************************/
	/*************Game Setting************/
	/*************************************/
	UPROPERTY(meta = (BindWidget))
	USlider* MouseSensitivitySlider;

	UPROPERTY(meta = (BindWidget))
	UEditableText* MouseSensitivityValueText;

	UFUNCTION()
	void MouseSensitivityChanged(float Value);

	UFUNCTION()
	void MouseSensitivityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void MouseSensitivitySliderReleased();
};
