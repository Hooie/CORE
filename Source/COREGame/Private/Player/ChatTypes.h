// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChatTypes.generated.h"

UENUM(BlueprintType)
enum class EChatChannel : uint8
{
	Team,
	All
};

USTRUCT(BlueprintType)
struct FChatMessage
{
	GENERATED_BODY()

public:
	FChatMessage();

	UPROPERTY(BlueprintReadOnly)
	FString SenderNickname;

	UPROPERTY(BlueprintReadOnly)
	FString MessageText;

	UPROPERTY(BlueprintReadOnly)
	EChatChannel Channel;

	UPROPERTY(BlueprintReadOnly)
	uint8 SenderTeamId;

	UPROPERTY(BlueprintReadOnly)
	int32 Hour;

	UPROPERTY(BlueprintReadOnly)
	int32 Minute;

	UPROPERTY(BlueprintReadOnly)
	int32 Second;
};