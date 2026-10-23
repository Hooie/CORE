// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamagePopupWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnFadeOutFinished);

UENUM(BlueprintType)
enum class EDamagePopupDirection : uint8
{
	Neutral,	// 제 3자간 전투
	Incoming,	// 내가 맞았을 때
	OutGoing,	// 내가 때렸을 때
};

/**
 * 
 */
UCLASS()
class UDamagePopupWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	FOnFadeOutFinished OnFadeOutFinished;

	void SetDamageValue(float DamageValue);
	void SetDamageType(bool bIsMagical);
	void PlayFadeOut();
	void SetDamageDirection(EDamagePopupDirection Direction, bool bIsItemBonus = false);
	void SetIsHeal(bool bIsLifeSteal);

private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DamageText;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor PhysicalDamageColor = FLinearColor(1.f, 0.15f, 0.15f);

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor MagicalDamageColor = FLinearColor(0.2f, 0.4f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor HealColor = FLinearColor(0.2f, 1.f, 0.3f);

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor LifeStealColor = FLinearColor(0.7f, 0.1f, 0.3f);

	UPROPERTY(EditDefaultsOnly, Category = "Visual|Direction")
	int32 IncomingFontSize = 33;

	UPROPERTY(EditDefaultsOnly, Category = "Visual|Direction")
	int32 OutgoingFontSize = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Visual|Direction")
	int32 NeutralFontSize = 16;

	UPROPERTY(EditDefaultsOnly, Category = "Visual|Direction", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ItemBonusFontScale = 0.5f;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* FadeOutAnimation;

	UFUNCTION()
	void HandleFadeOutAnimationFinished();
};
