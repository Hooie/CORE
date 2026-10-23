// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/VolumeSettingRow.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/EditableText.h"
#include "Framework/CGameUserSettings.h"

void UVolumeSettingRow::NativePreConstruct()
{
	Super::NativePreConstruct();

	if ( NameText )
		NameText->SetText(DisplayName);
}

void UVolumeSettingRow::NativeConstruct()
{
	Super::NativeConstruct();

	VolumeSlider->SetMinValue(0.f);
	VolumeSlider->SetMaxValue(1.f);

	if ( UCGameUserSettings* Settings = UCGameUserSettings::Get() )
	{
		const float Current = Settings->GetVolume(Category);
		VolumeSlider->SetValue(Current);
		UpdateValueText(Current);
	}

	VolumeSlider->OnValueChanged.RemoveAll(this);
	VolumeSlider->OnValueChanged.AddDynamic(this, &UVolumeSettingRow::OnSliderValueChanged);
	VolumeSlider->OnMouseCaptureEnd.RemoveAll(this);
	VolumeSlider->OnMouseCaptureEnd.AddDynamic(this, &UVolumeSettingRow::OnSliderReleased);
	VolumeSlider->OnControllerCaptureEnd.RemoveAll(this);
	VolumeSlider->OnControllerCaptureEnd.AddDynamic(this, &UVolumeSettingRow::OnSliderReleased);

	ValueText->OnTextCommitted.RemoveAll(this);
	ValueText->OnTextCommitted.AddDynamic(this, &UVolumeSettingRow::OnValueTextCommitted);
}

void UVolumeSettingRow::OnSliderValueChanged(float Value)
{
	if ( UCGameUserSettings* Settings = UCGameUserSettings::Get() )
		Settings->SetVolume(Category, Value, this);

	UpdateValueText(Value);
}

void UVolumeSettingRow::OnSliderReleased()
{
	if ( UCGameUserSettings* Settings = UCGameUserSettings::Get() )
		Settings->SaveSettings();
}

void UVolumeSettingRow::OnValueTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if ( CommitMethod == ETextCommit::OnCleared )
		return;

	UCGameUserSettings* Settings = UCGameUserSettings::Get();
	if ( !Settings )
		return;

	FString Input = Text.ToString().Replace(TEXT("%"), TEXT("")).TrimStartAndEnd();

	if ( !Input.IsNumeric() )
	{
		UpdateValueText(Settings->GetVolume(Category));
		return;
	}

	const float Value = FMath::Clamp(FCString::Atof(*Input) / 100.f, 0.f, 1.f);

	VolumeSlider->SetValue(Value);
	Settings->SetVolume(Category, Value, this);
	Settings->SaveSettings();

	UpdateValueText(Value);
}

void UVolumeSettingRow::UpdateValueText(float Value)
{
	ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), Value * 100.f)));
}