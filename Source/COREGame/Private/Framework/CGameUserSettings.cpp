// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameUserSettings.h"
#include "Framework/CAudioSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"

UCGameUserSettings* UCGameUserSettings::Get()
{
	return Cast<UCGameUserSettings>(GEngine->GetGameUserSettings());
}

void UCGameUserSettings::ApplyAndSaveOption()
{
	ApplySettings(false);
	SaveSettings();
}

float* UCGameUserSettings::GetVolumeRef(ESoundVolumeCategory Category)
{
	switch ( Category )
	{
	case ESoundVolumeCategory::Master: 
		return &MasterVolume;

	case ESoundVolumeCategory::Music:  
		return &MusicVolume;
	case ESoundVolumeCategory::SFX:    
		return &SFXVolume;

	case ESoundVolumeCategory::Voice:  
		return &VoiceVolume;

	case ESoundVolumeCategory::UI:     
		return &UIVolume;
	}

	return nullptr;
}

void UCGameUserSettings::SetMouseSensitivity(float NewSensitivity)
{
	MouseSensitivity = NewSensitivity;
	OnMouseSensitivityChanged.Broadcast(MouseSensitivity);
}

float UCGameUserSettings::GetVolume(ESoundVolumeCategory Category) const
{
	const float* Value = const_cast< UCGameUserSettings* >(this)->GetVolumeRef(Category);
	return Value ? *Value : 1.f;
}

void UCGameUserSettings::SetVolume(ESoundVolumeCategory Category, float NewVolume, const UObject* WorldContext)
{
	if ( float* Value = GetVolumeRef(Category) )
	{
		*Value = FMath::Clamp(NewVolume, 0.f, 1.f);
		ApplyAudioVolumes(WorldContext);
	}
}

void UCGameUserSettings::ApplyAudioVolumes(const UObject* WorldContext) const
{
	if ( IsRunningDedicatedServer() || !WorldContext )
		return;

	const UCAudioSettings* AudioSettings = GetDefault<UCAudioSettings>();
	USoundMix* Mix = AudioSettings->UserVolumeMix.LoadSynchronous();
	if ( !Mix )
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] UserVolumeMix가 Project Settings에 지정되지 않았습니다."));
		return;
	}

	for ( const TPair<ESoundVolumeCategory, TSoftObjectPtr<USoundClass>>& Pair : AudioSettings->SoundClasses )
	{
		USoundClass* SoundClass = Pair.Value.LoadSynchronous();
		if ( !SoundClass )
			continue;

		const bool bApplyToChildren = (Pair.Key == ESoundVolumeCategory::Master);

		UGameplayStatics::SetSoundMixClassOverride(WorldContext, Mix, SoundClass, GetVolume(Pair.Key), 1.f, 0.f, bApplyToChildren);
	}

	UGameplayStatics::PushSoundMixModifier(WorldContext, Mix);
}

