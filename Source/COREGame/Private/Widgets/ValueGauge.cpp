// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ValueGauge.h"
#include "AbilitySystemComponent.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Materials/MaterialInstanceDynamic.h"

void UValueGauge::NativePreConstruct()
{
	Super::NativePreConstruct();

	ValueText->SetFont(ValueTextFont);
	ValueText->SetVisibility(bValueTextVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

	ApplyHealthBarColor();

	if ( ShieldFillImage )
	{
		ShieldFillImage->SetColorAndOpacity(ShieldBarColor);
	}
	if ( ShieldValueText )
	{
		ShieldValueText->SetColorAndOpacity(ShieldValueTextColor);
	}

	if ( TrailFillImage )
	{
		TrailFillImage->SetColorAndOpacity(DamageTrailColor);
	}

	if ( TrailSegmentBox )
	{
		TrailSegmentBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if ( TickOverlayImage )
	{
		TickOverlayImage->SetVisibility(bShowTicks ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UValueGauge::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if ( !bEnableDamageTrail || !TrailSegmentBox )
		return;

	const float Filled = GetFilledValue();
	if ( TrailFilledValue <= Filled )
		return;

	if ( TrailHoldRemaining > 0.f )
	{
		TrailHoldRemaining -= InDeltaTime;
		return;
	}

	const float EffectiveMax = FMath::Max(CachedMaxValue, Filled);
	TrailFilledValue = FMath::FInterpConstantTo(TrailFilledValue, Filled, InDeltaTime, EffectiveMax * DamageTrailDrainRate);

	UpdateSegments();
}

void UValueGauge::SetAndBoundToGameplayAttribute(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute, const FGameplayAttribute& RegenAttribute, const FGameplayAttribute& ShieldAttribute)
{
	if ( AbilitySystemComponent )
	{
		bool bFound;
		float Value = AbilitySystemComponent->GetGameplayAttributeValue(Attribute, bFound);
		float MaxValue = AbilitySystemComponent->GetGameplayAttributeValue(MaxAttribute, bFound);

		if ( bFound )
		{
			SetValue(Value, MaxValue);
		}

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UValueGauge::ValueChanged);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxAttribute).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxAttribute).AddUObject(this, &UValueGauge::MaxValueChanged);

		if ( RegenAttribute.IsValid() )
		{
			bool bRegenAttributeFound = false;

			float RegenValue = AbilitySystemComponent->GetGameplayAttributeValue(RegenAttribute, bRegenAttributeFound);
			if ( bRegenAttributeFound )
			{
				SetRegenValue(RegenValue);
			}

			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(RegenAttribute).RemoveAll(this);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(RegenAttribute).AddUObject(this, &UValueGauge::RegenValueChanged);
		}

		if ( ShieldAttribute.IsValid() )
		{
			bool bShieldAttributeFound = false;
			float ShieldValue = AbilitySystemComponent->GetGameplayAttributeValue(ShieldAttribute, bShieldAttributeFound);
			if ( bShieldAttributeFound )
			{
				SetShieldValue(ShieldValue);
			}

			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ShieldAttribute).RemoveAll(this);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ShieldAttribute).AddUObject(this, &UValueGauge::ShieldValueChanged);
		}
	}
}

void UValueGauge::SetValue(float NewValue, float NewMaxValue)
{
	CachedValue = NewValue;
	CachedMaxValue = NewMaxValue;

	if ( NewMaxValue == 0 )
	{
		UE_LOG(LogTemp, Warning, TEXT("Value Gauge : %s, NewMaxValue Can't be 0"), *GetName());
		return;
	}

	UpdateTrailTarget();
	UpdateValueText();
	UpdateRegenVisibility();
	UpdateSegments();
}

void UValueGauge::SetRegenValue(float NewRegenValue)
{
	CachedRegenValue = NewRegenValue;

	if ( RegenValueText )
	{
		FNumberFormattingOptions FormatOps = FNumberFormattingOptions().SetMaximumFractionalDigits(1);

		RegenValueText->SetText(FText::Format(FTextFormat::FromString("+{0}"), FText::AsNumber(NewRegenValue / 5.f, &FormatOps)));
	}

	UpdateRegenVisibility();
}

void UValueGauge::SetShieldValue(float NewShieldValue)
{
	CachedShieldValue = NewShieldValue;

	UpdateTrailTarget();
	UpdateValueText();
	UpdateSegments();
}

void UValueGauge::SetHealthBarColorOverride(bool bOverride, const FLinearColor& OverrideColor)
{
	if ( bOverride )
	{
		OverrideHealthColor = OverrideColor;
	}
	else
	{
		OverrideHealthColor.Reset();
	}

	ApplyHealthBarColor();
}

void UValueGauge::SetHealthBarBaseColor(const FLinearColor& NewColor)
{
	RuntimeHealthColor = NewColor;
	ApplyHealthBarColor();
}

void UValueGauge::ResetHealthBarBaseColor()
{
	RuntimeHealthColor.Reset();
	ApplyHealthBarColor();
}

void UValueGauge::ValueChanged(const FOnAttributeChangeData& ChangedData)
{
	SetValue(ChangedData.NewValue, CachedMaxValue);
}

void UValueGauge::MaxValueChanged(const FOnAttributeChangeData& ChangedData)
{
	SetValue(CachedValue, ChangedData.NewValue);
}

void UValueGauge::RegenValueChanged(const FOnAttributeChangeData& ChangedData)
{
	SetRegenValue(ChangedData.NewValue);
}

void UValueGauge::ShieldValueChanged(const FOnAttributeChangeData& ChangedData)
{
	SetShieldValue(ChangedData.NewValue);
}

void UValueGauge::UpdateRegenVisibility()
{
	if ( !RegenValueText )
		return;

	bool bIsFull = (CachedMaxValue > 0.f) && (CachedValue >= CachedMaxValue);

	RegenValueText->SetVisibility((!bValueTextVisible || bIsFull || CachedRegenValue <= 0.f) ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}

void UValueGauge::UpdateSegments()
{
	if ( !HealthSegmentBox || !ShieldSegmentBox || !EmptySegmentBox || CachedMaxValue <= 0.f )
		return;

	const float Filled = CachedValue + CachedShieldValue;
	const float EffectiveMax = FMath::Max(CachedMaxValue, Filled);

	UpdateTicks(EffectiveMax);

	const bool bUseTrail = bEnableDamageTrail && TrailSegmentBox;
	const float TrailAmount = bUseTrail ? FMath::Clamp(TrailFilledValue - Filled, 0.f, EffectiveMax - Filled) : 0.f;

	const float HealthRatio = CachedValue / EffectiveMax;
	const float ShieldRatio = CachedShieldValue / EffectiveMax;
	const float TrailRatio = TrailAmount / EffectiveMax;
	const float EmptyRatio = FMath::Max(0.f, 1.f - HealthRatio - ShieldRatio - TrailRatio);

	auto SetFillSize = [] (USizeBox* Box, float Ratio)
		{
			if ( UHorizontalBoxSlot* BoxSlot = Box ? Cast<UHorizontalBoxSlot>(Box->Slot) : nullptr )
			{
				FSlateChildSize Size;
				Size.SizeRule = ESlateSizeRule::Fill;
				Size.Value = FMath::Max(Ratio, 0.0001f);
				BoxSlot->SetSize(Size);
			}
		};

	SetFillSize(HealthSegmentBox, HealthRatio);
	SetFillSize(ShieldSegmentBox, ShieldRatio);
	SetFillSize(EmptySegmentBox, EmptyRatio);

	ShieldSegmentBox->SetVisibility(CachedShieldValue > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if ( TrailSegmentBox )
	{
		SetFillSize(TrailSegmentBox, TrailRatio);
		TrailSegmentBox->SetVisibility(TrailAmount > KINDA_SMALL_NUMBER ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UValueGauge::UpdateValueText()
{
	if ( !ValueText || CachedMaxValue <= 0.f )
		return;

	FNumberFormattingOptions FormatOps = FNumberFormattingOptions().SetMaximumFractionalDigits(0);

	ValueText->SetText(FText::Format(FTextFormat::FromString("{0} / {1}"), FText::AsNumber(CachedValue, &FormatOps), FText::AsNumber(CachedMaxValue, &FormatOps)));

	if ( ShieldValueText )
	{
		if ( CachedShieldValue > 0.f && bValueTextVisible ) 
		{
			ShieldValueText->SetText(FText::Format(FTextFormat::FromString(" (+ {0})"), FText::AsNumber(CachedShieldValue, &FormatOps)));
			ShieldValueText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ShieldValueText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UValueGauge::ApplyHealthBarColor()
{
	if ( !HealthFillImage )
		return;

	HealthFillImage->SetColorAndOpacity(OverrideHealthColor.Get(RuntimeHealthColor.Get(HealthBarColor)));
}

void UValueGauge::UpdateTrailTarget()
{
	const float Filled = GetFilledValue();
	const bool bDropped = bTrailInitialized && Filled < LastFilledValue;
	LastFilledValue = Filled;

	if ( !bTrailInitialized || !bEnableDamageTrail || Filled > TrailFilledValue )
	{
		TrailFilledValue = Filled;
		TrailHoldRemaining = 0.f;
		bTrailInitialized = true;
		return;
	}

	if ( bDropped )
	{
		TrailHoldRemaining = DamageTrailHoldDelay;
	}
}

void UValueGauge::UpdateTicks(float EffectiveMax)
{
	if ( !bShowTicks || !TickOverlayImage )
		return;

	const float SegmentCount = EffectiveMax / TickUnit;

	if ( FMath::IsNearlyEqual(SegmentCount, LastTickSegmentCount) )
		return;

	if ( UMaterialInstanceDynamic* TickMID = TickOverlayImage->GetDynamicMaterial() )
	{
		TickMID->SetScalarParameterValue(TEXT("SegmentCount"), SegmentCount);
		LastTickSegmentCount = SegmentCount;
	}
}