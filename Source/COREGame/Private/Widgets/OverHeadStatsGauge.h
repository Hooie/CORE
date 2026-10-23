// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "OverHeadStatsGauge.generated.h"

/**
 * 
 */
UCLASS()
class UOverHeadStatsGauge : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureWithASC(class UAbilitySystemComponent* AbilitySystemComponent);
	
	void SetDistanceScaling(float InMinDistance, float InMaxDistance, float InNearScale, float InFarScale);

	void SetTeamRelation(int32 Relation);
	void SetPlayerName(const FString& PlayerName);

	void SnapDistanceScale();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UAbilitySystemComponent* GetOwnerASC() const { return OwnerASC.Get(); }
	AActor* GetOwnerActor() const { return OwnerActor.Get(); }

	UPROPERTY(meta = (BindWidget))
	class UValueGauge* HealthBar;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	class UValueGauge* ManaBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* LevelText;

	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;
	TWeakObjectPtr<AActor> OwnerActor;

	void OnLevelChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NameText;

	UPROPERTY(EditAnywhere, Category = "Team Color")
	bool bUseTeamHealthColor = true;

	UPROPERTY(EditAnywhere, Category = "Team Color")
	FLinearColor AllyHealthColor = FLinearColor(0.25f, 0.6f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Team Color")
	FLinearColor EnemyHealthColor = FLinearColor(0.9f, 0.2f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Team Color")
	FLinearColor SelfHealthColor = FLinearColor(0.35f, 0.85f, 0.35f);

	/************************************/
	/***********Distance Scale***********/
	/************************************/
	bool bDistanceScalingEnabled = false;
	float MinScaleDistance = 800.f;
	float MaxScaleDistance = 6000.f;
	float NearScale = 1.f;
	float FarScale = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Distance Scale")
	float ScaleInterpSpeed = 12.f;

	float CurrentScale = -1.f;
	float LastScalUpdateTime = -1.f;

	void UpdateDistanceScale(float DeltaTime);

	bool ComputeTargetScale(float& OutScale) const;
};
