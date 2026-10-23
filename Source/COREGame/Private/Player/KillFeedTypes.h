// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KillFeedTypes.generated.h"

USTRUCT(BlueprintType)
struct FKillFeedEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString KillerName;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> KillerIcon;

	UPROPERTY(BlueprintReadOnly)
	uint8 KillerTeamId = 0;

	UPROPERTY(BlueprintReadOnly)
	FString VictimName;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> VictimIcon;

	UPROPERTY(BlueprintReadOnly)
	uint8 VictimTeamId = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<TSoftObjectPtr<UTexture2D>> AssistIcons;
};