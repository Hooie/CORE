// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "GameplayTagContainer.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Widgets/ItemWidget.h"
#include "Widgets/TreeNodeInterface.h"
#include "ShopItemWidget.generated.h"

class UPA_ShopItem;
class UShopItemWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemPurchaseIssued, const UPA_ShopItem*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShopItemSelected, const UShopItemWidget*);

/**
 * 
 */
UCLASS()
class UShopItemWidget : public UItemWidget, public IUserObjectListEntry, public ITreeNodeInterface
{
	GENERATED_BODY()
	
public:
	FOnItemPurchaseIssued OnItemPurchaseIssued;
	FOnShopItemSelected OnShopItemClicked;

	virtual UUserWidget* GetWidget() const override;
	virtual TArray<const ITreeNodeInterface*> GetInputs() const override;
	virtual TArray<const ITreeNodeInterface*> GetOutputs() const override;
	virtual const UObject* GetItemObject() const override;

	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	FORCEINLINE const UPA_ShopItem* GetShopItem() const { return ShopItem; }

	void RefreshState(); // 가격 / 아이콘 색 / 보유 표시 갱신 함수
	void ApplyTreeOwnership(bool bShowOwned, bool bDimmed); // 조합 트리 전용 갱신 함수

private:
	void CopyFromOther(const UShopItemWidget* OtherWidget);
	void InitWithShopItem(const UPA_ShopItem* NewShopItem);

	TArray<const ITreeNodeInterface*> ItemsToInterfaces(const TArray<const UPA_ShopItem*>& Items) const;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PriceText;

	UPROPERTY()
	const UPA_ShopItem* ShopItem;

	UPROPERTY()
	UItemToolTip* CachedToolTip;

	UPROPERTY(meta=(BindWidget))
	class UBorder* OwnedHighlightBorder;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor UnaffordableIconColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor UnaffordablePriceColor = FLinearColor(1.f, 0.25f, 0.25f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor AffordablePriceColor = FLinearColor(1.f, 0.85f, 0.2f, 1.f);

	const class UListView* ParentListView;
		 
	virtual void RightButtonClicked() override;
	virtual void LeftButtonClicked() override;
		
	class UInventoryComponent* GetOwnerInventory() const;
};
