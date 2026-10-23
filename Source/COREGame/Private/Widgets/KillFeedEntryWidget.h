// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/KillFeedTypes.h"
#include "KillFeedEntryWidget.generated.h"

/**
 * 
 */
UCLASS()
class UKillFeedEntryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void ConfigureKillFeedEntry(const FKillFeedEntry& Entry);
	void PlayVanishAndRemove();
	void PlayAppearAnim();

private:
	UPROPERTY(meta = (BindWidget))
	class UImage* KillerIconImage;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* KillerNameText;

	UPROPERTY(meta = (BindWidget))
	class UImage* VictimIconImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* VictimNameText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* VanishAnim;

	UFUNCTION()
	void OnVanishAnimFinished();

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* AppearAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Kill Feed")
	FLinearColor AllyColor = FLinearColor(0.2f, 0.9f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category = "Kill Feed")
	FLinearColor EnemyColor = FLinearColor(0.9f, 0.2f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category = "Kill Feed")
	FVector2D AssistSlotOverrideSize = FVector2D(20.f, 20.f);

	uint8 GetLocalTeamId() const;

	UPROPERTY(meta=(BindWidget))
	class UVerticalBox* AssistIconContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Kill Feed")
	TSubclassOf<class UUserWidget> AssistIconWidgetClass;
};
