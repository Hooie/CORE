// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/OverHeadStatsGauge.h"
#include "MinionOverheadGauge.generated.h"

/**
 * 
 */
UCLASS()
class UMinionOverheadGauge : public UOverHeadStatsGauge
{
	GENERATED_BODY()
	

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(EditAnywhere, Category = "Last Hit")
	FLinearColor LastHitColor = FLinearColor(1.f, 0.85f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Last Hit")
	bool bOnlyEnemyMinion = true;

	UPROPERTY(EditAnywhere, Category = "Last Hit")
	float LastHitCheckInterval = 0.05f;

	float TimeSinceLastCheck = 0.f;
	bool bIsLastHitHighlighted = false;

	void UpdateLastHitHighlight();
	void SetLastHitHightlighted(bool bHighlighted);
};
