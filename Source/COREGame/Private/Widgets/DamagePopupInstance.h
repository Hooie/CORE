// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Widgets/DamagePopupWidget.h"
#include "DamagePopupInstance.generated.h"

UCLASS()
class ADamagePopupInstance : public AActor
{
	GENERATED_BODY()
	
public:	
	ADamagePopupInstance();

	void Show(TSubclassOf<UDamagePopupWidget> WidgetClass, float RawMagnitude, bool bIsHealCue, bool bSecondaryFlag, EDamagePopupDirection Direction, bool bIsItemBonus = false);

private:
	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* DamageTextComponent;

	UPROPERTY()
	UDamagePopupWidget* CachedWidget;
	
	UFUNCTION()
	void OnWidgetFadeOutFinished();
};
