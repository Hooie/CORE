// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Framework/CAudioSettings.h"
#include "CGameUserSettings.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMouseSensitivityChanged, float);

/**
 * 
 */
UCLASS()
class UCGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	FOnMouseSensitivityChanged OnMouseSensitivityChanged;

	static UCGameUserSettings* Get();

	void SetMouseSensitivity(float NewSensitivity);
	
	float GetVolume(ESoundVolumeCategory Category) const;
	void SetVolume(ESoundVolumeCategory Category, float NewVolume, const UObject* WorldContext);

	void ApplyAudioVolumes(const UObject* WorldContext) const;

	UPROPERTY(Config)
	float MouseSensitivity = 0.5f;

	UPROPERTY(Config)
	float MasterVolume = 0.4f;
	
	UPROPERTY(Config)
	float MusicVolume = 1.f;

	UPROPERTY(Config)
	float SFXVolume = 1.f;

	UPROPERTY(Config)
	float VoiceVolume = 1.f;

	UPROPERTY(Config)
	float UIVolume = 1.f;
	
	void ApplyAndSaveOption();

private:
	float* GetVolumeRef(ESoundVolumeCategory Category);
};
