// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "ItemStatRow.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class UItemStatRow : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetStat(const struct FItemStatEntry& Entry, const FText& Label);

protected:
	/** 값이 채워진 직후 호출. 스탯별 색/아이콘/애니메이션 등을 추가로 처리하고 싶을 때 블루프린트에서 구현 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Stat")
	void OnStatSet(const FGameplayAttribute& Attribute, const FText& Label, float Magnitude, bool bIsPercent);

	UPROPERTY(EditDefaultsOnly, Category = "Stat|Style")
	TMap<FGameplayAttribute, FSlateColor> AttributeValueColors;

private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ValueText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AttributeText;
};
