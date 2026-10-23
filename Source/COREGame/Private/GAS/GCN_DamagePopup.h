// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_DamagePopup.generated.h"

/**
 * 
 */
UCLASS()
class AGCN_DamagePopup : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	AGCN_DamagePopup();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSubclassOf<class UDamagePopupWidget> DamageWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSubclassOf<class ADamagePopupInstance> PopupInstanceClass;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	bool bIsHealCue = false;
};
