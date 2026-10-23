// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HoldProgressWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHoldProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FormatOps.SetMaximumFractionalDigits(1);
}

void UHoldProgressWidget::StartHold(float InMaxHoldTime, bool bCountDown)
{
	MaxHoldTime = InMaxHoldTime;
	StartTime = GetWorld()->GetTimeSeconds();
	bIsCountDown = bCountDown;

	SetVisibility(ESlateVisibility::Visible);

	float InitialPercent = bIsCountDown ? 1.f : 0.f;
	float InitialTimeValue = bIsCountDown ? MaxHoldTime : 0.f;

	if ( HoldProgressBar )
		HoldProgressBar->SetPercent(0.f);

	if ( HoldTimeText )
		HoldTimeText->SetText(FText::AsNumber(0.f, &FormatOps));

	GetWorld()->GetTimerManager().SetTimer(UpdateTimerHandle, this, &UHoldProgressWidget::UpdateProgress, 0.05f, true);
}

void UHoldProgressWidget::StopHold()
{
	GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UHoldProgressWidget::UpdateProgress()
{
	if ( MaxHoldTime <= 0.f )
		return;

	float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;
	float ClampedElapsed = FMath::Min(ElapsedTime, MaxHoldTime);
	float RiseRatio = FMath::Clamp(ElapsedTime / MaxHoldTime, 0.f, 1.f);

	float Percent = bIsCountDown ? (1.f - RiseRatio) : RiseRatio;
	float DisplayTimeValue = bIsCountDown ? (MaxHoldTime - ClampedElapsed) : ClampedElapsed;

	if ( HoldProgressBar )
		HoldProgressBar->SetPercent(Percent);

	if ( HoldTimeText )
		HoldTimeText->SetText(FText::AsNumber(DisplayTimeValue, &FormatOps));

	if ( RiseRatio >= 1.f )
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}
}