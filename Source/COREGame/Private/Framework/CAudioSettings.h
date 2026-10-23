// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CAudioSettings.generated.h"

class USoundMix;
class USoundClass;

UENUM(BlueprintType)
enum class ESoundVolumeCategory : uint8
{
	Master,
	Music,
	SFX,
	Voice,
	UI
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Audio Volume"))
class UCAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> UserVolumeMix;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TMap<ESoundVolumeCategory, TSoftObjectPtr<USoundClass>> SoundClasses;
};
