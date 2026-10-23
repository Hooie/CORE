// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffect.h"
#include "BuffTrayWidget.generated.h"

/**
 * 
 */
UCLASS()
class UBuffTrayWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* BuffContainer;

	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* DebuffContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	TSubclassOf<class UBuffIconWidget> BuffIconWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	class UDataTable* BuffIconDataTable;

	UPROPERTY()
	TMap<FActiveGameplayEffectHandle, class UBuffIconWidget*> ActiveIcons;

	void OnEffectApplied(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void OnEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo, FActiveGameplayEffectHandle Handle);

	void OnEffectTimeChanged(FActiveGameplayEffectHandle Handle, float NewStartTime, float NewDuration);
	void OnEffectStackChanged(FActiveGameplayEffectHandle Handle, int32 NewStackCount, int32 PreviousStackCount);
};

