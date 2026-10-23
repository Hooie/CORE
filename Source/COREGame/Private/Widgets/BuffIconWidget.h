// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuffIconWidget.generated.h"

/**
 * 
 */
UCLASS()
class UBuffIconWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void Configure(UTexture2D* Icon, float StartTime, float Duration, bool bIsDebuff); // Duration <= 0이면 무기한 버프
	void UpdatePercent();

	void RefreshTime(float StartTime, float Duration);

	void SetStackCount(int32 NewStackCount);

protected:
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UImage* IconImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* RemainingTimeText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* StackCountText;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName IconMaterialParamName = "Icon";

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName IsDebuffParamName = "IsDebuff";

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName RemainingPercentParamName = "Percent";

	float EffectStartTime = 0.f;
	float EffectDuration = -1.f;

	FTimerHandle UpdateTimerHandle;
	
	
};
