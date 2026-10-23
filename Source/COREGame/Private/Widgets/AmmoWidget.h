// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AmmoWidget.generated.h"

/**
 * 
 */
UCLASS()
class UAmmoWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UImage* GunIcon;

	UPROPERTY(meta=(BindWidget))
	class UTextBlock* AmmoText;

	void OnAmmoChanged(int32 CurrentAmmo, int32 MaxAmmo);
};
