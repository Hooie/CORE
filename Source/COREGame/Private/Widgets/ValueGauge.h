// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "ValueGauge.generated.h"

/**
 * 
 */
UCLASS()
class UValueGauge : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetAndBoundToGameplayAttribute(class UAbilitySystemComponent* AbilitySystemComponent, const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute, const FGameplayAttribute& RegenAttribute = FGameplayAttribute(), const FGameplayAttribute& ShieldAttribute = FGameplayAttribute());
	void SetValue(float NewValue, float NewMaxValue);
	void SetRegenValue(float NewRegenValue);
	void SetShieldValue(float NewShieldValue);

	void SetHealthBarColorOverride(bool bOverride, const FLinearColor& OverrideColor = FLinearColor::White);

	void SetHealthBarBaseColor(const FLinearColor& NewColor);
	void ResetHealthBarBaseColor();

private:
	void ValueChanged(const FOnAttributeChangeData& ChangedData);
	void MaxValueChanged(const FOnAttributeChangeData& ChangedData);

	void RegenValueChanged(const FOnAttributeChangeData& ChangedData);
	void ShieldValueChanged(const FOnAttributeChangeData& ChangedData);
	void UpdateRegenVisibility();

	void UpdateSegments();

	float CachedValue;
	float CachedMaxValue;
	float CachedRegenValue = 0.f;
	float CachedShieldValue = 0.f;

	UPROPERTY(EditAnywhere, Category = "Visual")
	FLinearColor HealthBarColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, Category = "Visual")
	FLinearColor ShieldBarColor = FLinearColor(0.6f, 0.6f, 0.6f, 0.8f);

	UPROPERTY(EditAnywhere, Category = "Visual")
	FLinearColor ShieldValueTextColor = FLinearColor(0.7f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Visual")
	FSlateFontInfo ValueTextFont;

	UPROPERTY(EditAnywhere, Category = "Visual")
	bool bValueTextVisible = true;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class UTextBlock* RegenValueText;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class UTextBlock* ValueText;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class UTextBlock* ShieldValueText;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class USizeBox* HealthSegmentBox;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class USizeBox* ShieldSegmentBox;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class USizeBox* EmptySegmentBox;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class UImage* HealthFillImage;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class UImage* ShieldFillImage;

	void UpdateValueText();

	// 팀 색 (비어 있으면 HealthBarColor 사용)
	TOptional<FLinearColor> RuntimeHealthColor;

	// 막타 하이라이트 등 최우선 색 (비어 있으면 미사용)
	TOptional<FLinearColor> OverrideHealthColor;

	void ApplyHealthBarColor();

	/****************************/
	/********Damage Trail********/
	/****************************/
	UPROPERTY(EditAnywhere, Category = "Visual|Damage Trail")
	bool bEnableDamageTrail = true;

	UPROPERTY(EditAnywhere, Category = "Visual|Damage Trail")
	FLinearColor DamageTrailColor = FLinearColor(1.f, 0.85f, 0.55f, 0.9f);

	UPROPERTY(EditAnywhere, Category = "Visual|Damage Trail", meta = (ClampMin = "0.0"))
	float DamageTrailHoldDelay = 0.5f;
	
	UPROPERTY(EditAnywhere, Category = "Visual|Damage Trail", meta = (ClampMin = "0.01"))
	float DamageTrailDrainRate = 0.6f;

	UPROPERTY(VisibleAnywhere, meta = (BindWidgetOptional))
	class USizeBox* TrailSegmentBox;

	UPROPERTY(VisibleAnywhere, meta = (BindWidgetOptional))
	class UImage* TrailFillImage;

	bool bTrailInitialized = false;
	float TrailFilledValue = 0.f;
	float LastFilledValue = 0.f;
	float TrailHoldRemaining = 0.f;

	void UpdateTrailTarget();
	float GetFilledValue() const { return FMath::Max(0.f, CachedValue + CachedShieldValue); }

	/****************************/
	/********Health Ticks********/
	/****************************/
	// 체력바 인스턴스에서만 켜기 (마나바는 끔)
	UPROPERTY(EditAnywhere, Category = "Visual|Ticks")
	bool bShowTicks = false;

	// 눈금 한 칸이 나타내는 값 (LoL: 100)
	UPROPERTY(EditAnywhere, Category = "Visual|Ticks", meta = (ClampMin = "1.0"))
	float TickUnit = 100.f;

	UPROPERTY(VisibleAnywhere, meta = (BindWidgetOptional))
	class UImage* TickOverlayImage;

	float LastTickSegmentCount = -1.f;

	void UpdateTicks(float EffectiveMax);
};
