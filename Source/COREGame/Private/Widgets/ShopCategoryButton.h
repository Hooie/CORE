// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ShopCategoryButton.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCategoryBtnClicked, const FGameplayTag& /*Category Tag*/);

/**
 * 
 */
UCLASS()
class UShopCategoryButton : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FOnCategoryBtnClicked OnCategoryBtnClicked;

	void InitCategory(const FGameplayTag& InTag, const FText& InDisplayName, UTexture2D* InIcon);
	void SetSelected(bool bSelected);

	const FGameplayTag& GetCategoryTag() const { return CategoryTag; }

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UButton* CategoryButton;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CategoryNameText;

	UPROPERTY(meta = (BindWidget))
	class UImage* CategoryIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor SelectedColor = FLinearColor(1.f, 0.8f, 0.3f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor NormalColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.f);

	FGameplayTag CategoryTag;

	UFUNCTION()
	void HandleClicked();
};
