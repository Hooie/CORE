// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/BuffIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UBuffIconWidget::Configure(UTexture2D* Icon, float StartTime, float Duration, bool bIsDebuff)
{
	if ( Icon )
	{
		IconImage->GetDynamicMaterial()->SetTextureParameterValue(IconMaterialParamName, Icon);
	}

	IconImage->GetDynamicMaterial()->SetScalarParameterValue(IsDebuffParamName, bIsDebuff ? 1.f : 0.f);

	RefreshTime(StartTime, Duration);
}

void UBuffIconWidget::UpdatePercent()
{
	UWorld* World = GetWorld();
	if ( !World )
		return;

	float Elapsed = World->GetTimeSeconds() - EffectStartTime;
	float RemainingTime = FMath::Max(0.f, EffectDuration - Elapsed);
	float Percent = EffectDuration > 0.f ? FMath::Clamp(RemainingTime / EffectDuration, 0.f, 1.f) : 1.f;

	IconImage->GetDynamicMaterial()->SetScalarParameterValue(RemainingPercentParamName, Percent);

	if ( RemainingTimeText )
	{
		RemainingTimeText->SetText(FText::AsNumber(FMath::CeilToInt(RemainingTime)));
	}
}

void UBuffIconWidget::RefreshTime(float StartTime, float Duration)
{
	EffectStartTime = StartTime;
	EffectDuration = Duration;

	UWorld* World = GetWorld();
	if ( !World )
		return;

	if ( EffectDuration > 0.f )
	{
		if ( !World->GetTimerManager().IsTimerActive(UpdateTimerHandle) )
		{
			World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &UBuffIconWidget::UpdatePercent, 0.1f, true);
		}
		
		UpdatePercent();
	}
	else
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);

		IconImage->GetDynamicMaterial()->SetScalarParameterValue(RemainingPercentParamName, 1.f);
		if ( RemainingTimeText )
		{
			RemainingTimeText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UBuffIconWidget::SetStackCount(int32 NewStackCount)
{
	if ( !StackCountText )
	{
		return;
	}

	if ( NewStackCount > 1 )
	{
		StackCountText->SetText(FText::Format(INVTEXT("x{0}"), NewStackCount));
		StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		StackCountText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBuffIconWidget::NativeDestruct()
{
	if ( UWorld* World = GetWorld() )
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}

	Super::NativeDestruct();
}
