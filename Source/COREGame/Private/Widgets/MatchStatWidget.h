// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchStatWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * 
 */
UCLASS()
class UMatchStatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
private:
	UPROPERTY(meta=(BindWidget))
	UTextBlock* TeamOneCountText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TeamTwoCountText;

	UPROPERTY(meta = (BindWidget))
	UImage* ProgressImage;

	UPROPERTY()
	class AStormCore* StormCore;

	void UpdateTeamInfluence(int TeamOneCount, int TeamTwoCount);
	void MatchFinished(AActor* ViewTarget, int WinningTeam);

	UPROPERTY(EditDefaultsOnly, Category = "Match Stat")
	float ProgressUpdateInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Match Stat")
	FName ProgressDynamicMaterialParameterName = "Progress";

	void UpdateProgress();

	FTimerHandle UpdateProgressTimerHandle;
};
