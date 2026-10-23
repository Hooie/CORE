// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/GameConfigWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Components/Slider.h"
#include "Framework/CGameUserSettings.h"

void UGameConfigWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Settings = UCGameUserSettings::Get();

	WindowModeComboBox->AddOption(TEXT("전체화면"));
	WindowModeComboBox->AddOption(TEXT("창모드"));
	WindowModeComboBox->AddOption(TEXT("테두리없는 창모드"));

	WindowModeComboBox->OnSelectionChanged.AddDynamic(this, &UGameConfigWidget::WindowModeSelectionChanged);

	ResolutionComboBox->AddOption(TEXT("3840x2160"));
	ResolutionComboBox->AddOption(TEXT("3440x1440"));
	ResolutionComboBox->AddOption(TEXT("2560x1440"));
	ResolutionComboBox->AddOption(TEXT("1920x1080"));
	ResolutionComboBox->AddOption(TEXT("1680x1050"));
	ResolutionComboBox->AddOption(TEXT("1440x900"));
	ResolutionComboBox->AddOption(TEXT("1280x720"));

	ResolutionComboBox->OnSelectionChanged.AddDynamic(this, &UGameConfigWidget::ResolutionSelectionChanged);

	QualityLeftArrowBtn->OnClicked.AddDynamic(this, &UGameConfigWidget::QualityLeftArrowBtnClicked);
	QualityRightArrowBtn->OnClicked.AddDynamic(this, &UGameConfigWidget::QualityRightArrowBtnClicked);
	QualityApplyBtn->OnClicked.AddDynamic(this, &UGameConfigWidget::QualityApplyBtnClicked);

	MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &UGameConfigWidget::MouseSensitivityChanged);
	MouseSensitivitySlider->OnMouseCaptureEnd.AddDynamic(this, &UGameConfigWidget::MouseSensitivitySliderReleased);
	MouseSensitivitySlider->OnControllerCaptureEnd.AddDynamic(this, &UGameConfigWidget::MouseSensitivitySliderReleased);

	MouseSensitivityValueText->OnTextCommitted.AddDynamic(this, &UGameConfigWidget::MouseSensitivityTextCommitted);

	ReturnBtn->OnClicked.AddDynamic(this, &UGameConfigWidget::ReturnBtnClicked);

	MouseSensitivitySlider->SetMinValue(0.1f);
	MouseSensitivitySlider->SetMaxValue(3.f);

	InitializeSettings();
}

void UGameConfigWidget::InitializeSettings()
{
	if ( !Settings )
		return;

	switch ( Settings->GetFullscreenMode() )
	{
	case EWindowMode::Fullscreen:
		WindowModeComboBox->SetSelectedOption(TEXT("전체화면"));
		break;

	case EWindowMode::Windowed:
		WindowModeComboBox->SetSelectedOption(TEXT("창모드"));
		CachedWindowResolution = Settings->GetScreenResolution();
		break;

	case EWindowMode::WindowedFullscreen:
		WindowModeComboBox->SetSelectedOption(TEXT("테두리없는 창모드"));
		break;
	}

	int CurrentLevel = Settings->GetOverallScalabilityLevel();
	PendingQualityLevel = FMath::Clamp(CurrentLevel, 0, 3);
	RefreshQualityDisplay();

	ApplyMouseLockMode(Settings->GetFullscreenMode());

	SyncResolutionComboBoxToCurrentResolution();
	
	UpdateResolutionComboBoxEnabled();

	MouseSensitivitySlider->SetValue(Settings->MouseSensitivity);
	MouseSensitivityValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Settings->MouseSensitivity)));
}

void UGameConfigWidget::ReturnBtnClicked()
{
	OnReturnBtnClicked.Broadcast();
}

void UGameConfigWidget::WindowModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if ( SelectionType == ESelectInfo::Direct )
		return;

	if ( !Settings )
		return;

	EWindowMode::Type TargetMode = EWindowMode::Windowed;

	if ( SelectedItem == TEXT("전체화면") )
	{
		TargetMode = EWindowMode::Fullscreen;

		Settings->SetFullscreenMode(EWindowMode::Fullscreen);

		FIntPoint NativeRes = Settings->GetDesktopResolution();
		Settings->SetScreenResolution(NativeRes);
	}

	else if ( SelectedItem == TEXT("창모드") )
	{
		TargetMode = EWindowMode::Windowed;

		Settings->SetFullscreenMode(EWindowMode::Windowed);

		FIntPoint TargetRes = CachedWindowResolution.IsSet() ? CachedWindowResolution.GetValue() : Settings->GetScreenResolution();

		CachedWindowResolution = TargetRes;

		Settings->SetScreenResolution(TargetRes);
	}

	else if ( SelectedItem == TEXT("테두리없는 창모드") )
	{
		TargetMode = EWindowMode::WindowedFullscreen;

		Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);

		FIntPoint NativeRes = Settings->GetDesktopResolution();
		Settings->SetScreenResolution(NativeRes);
	}

	ApplyMouseLockMode(TargetMode);

	SyncResolutionComboBoxToCurrentResolution();

	UpdateResolutionComboBoxEnabled();
	Settings->ApplyAndSaveOption();
}

void UGameConfigWidget::ResolutionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if ( SelectionType == ESelectInfo::Direct )
		return;

	if ( !Settings )
		return;

	TArray<FString> Parts;
	SelectedItem.ParseIntoArray(Parts, TEXT("x"));
	if ( Parts.Num() != 2 )
		return;

	FIntPoint NewRes = FIntPoint(FCString::Atoi(*Parts[ 0 ]), FCString::Atoi(*Parts[ 1 ]));

	CachedWindowResolution = NewRes;

	Settings->SetScreenResolution(NewRes);
	Settings->ApplyAndSaveOption();
}

void UGameConfigWidget::UpdateResolutionComboBoxEnabled()
{
	if ( !Settings )
		return;

	EWindowMode::Type Mode = Settings->GetFullscreenMode();

	bool bEnabled = (Mode == EWindowMode::Windowed);
	ResolutionComboBox->SetIsEnabled(bEnabled);
}

void UGameConfigWidget::SyncResolutionComboBoxToCurrentResolution()
{
	if ( !Settings )
		return;

	FIntPoint CurrentRes = Settings->GetScreenResolution();
	FString CurrentResStr = FString::Printf(TEXT("%dx%d"), CurrentRes.X, CurrentRes.Y);

	if ( ResolutionComboBox->FindOptionIndex(CurrentResStr) == -1 )
	{
		ResolutionComboBox->AddOption(CurrentResStr);
	}
	
	ResolutionComboBox->SetSelectedOption(CurrentResStr);
}

void UGameConfigWidget::QualityLeftArrowBtnClicked()
{
	ChangePendingQualityLevel(-1);
}

void UGameConfigWidget::QualityRightArrowBtnClicked()
{
	ChangePendingQualityLevel(1);
}

void UGameConfigWidget::QualityApplyBtnClicked()
{
	if ( !Settings )
		return;

	Settings->SetOverallScalabilityLevel(PendingQualityLevel);
	Settings->ApplyAndSaveOption();

	RefreshQualityDisplay();
}

void UGameConfigWidget::ChangePendingQualityLevel(int Delta)
{
	PendingQualityLevel = FMath::Clamp(PendingQualityLevel + Delta, 0, 3);
	RefreshQualityDisplay();
}

void UGameConfigWidget::RefreshQualityDisplay()
{
	QualityValueText->SetText(FText::FromString(GetQualityLevelLabel(PendingQualityLevel)));

	QualityLeftArrowBtn->SetIsEnabled(PendingQualityLevel > 0);
	QualityRightArrowBtn->SetIsEnabled(PendingQualityLevel < 3);

	if ( Settings )
	{
		bool bIsSameAsCurrent = (PendingQualityLevel == Settings->GetOverallScalabilityLevel());
		QualityApplyBtn->SetIsEnabled(!bIsSameAsCurrent);
	}
}

FString UGameConfigWidget::GetQualityLevelLabel(int Level) const
{
	switch ( Level )
	{
	case 0:
		return TEXT("낮음");
	case 1:
		return TEXT("보통");
	case 2:
		return TEXT("높음");
	case 3:
		return TEXT("최고");
	default:
		return TEXT("높음");
	}
}

void UGameConfigWidget::ApplyMouseLockMode(EWindowMode::Type Mode)
{
	if ( !GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport )
		return;

	bool bShouldLock = (Mode == EWindowMode::Fullscreen);

	GEngine->GameViewport->SetMouseLockMode(bShouldLock ? EMouseLockMode::LockAlways : EMouseLockMode::LockOnCapture);

	GEngine->GameViewport->Viewport->LockMouseToViewport(bShouldLock);
}

void UGameConfigWidget::MouseSensitivityChanged(float Value)
{
	if ( !Settings )
		return;

	Settings->SetMouseSensitivity(Value);

	MouseSensitivityValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Value)));
}

void UGameConfigWidget::MouseSensitivityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if ( CommitMethod == ETextCommit::OnCleared )
		return;

	float Value = FCString::Atof(*Text.ToString());
	Value = FMath::Clamp(Value, 0.1f, 3.f);

	MouseSensitivitySlider->SetValue(Value);
	MouseSensitivityValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Value)));

	if ( !Settings )
		return;

	Settings->SetMouseSensitivity(Value);
	Settings->SaveSettings();
}

void UGameConfigWidget::MouseSensitivitySliderReleased()
{
	if ( Settings )
		Settings->SaveSettings();
}

