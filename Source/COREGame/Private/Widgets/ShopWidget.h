// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Inventory/InventoryComponent.h"
#include "ShopWidget.generated.h"

class UTileView;
class UPA_ShopItem;
class UShopItemWidget;
class UInventoryComponent;

USTRUCT(BlueprintType)
struct FShopCategoryInfo
{
	GENERATED_BODY()

	// 비워두면 "전체" 버튼으로 동작
	UPROPERTY(EditAnywhere, meta = (Categories = "ItemCategory"))
	FGameplayTag CategoryTag;

	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere)
	UTexture2D* Icon = nullptr;
};

/**
 * 
 */
UCLASS()
class UShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	void SetShopOpen(bool bOpen);

private:
	UPROPERTY(meta=(BindWidget))
	UTileView* ShopItemList;

	UPROPERTY(meta = (BindWidget))
	class UItemTreeWidget* CombinationTree;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	class USoundBase* PurchaseSuccessSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	class USoundBase* SellSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	class USoundBase* FailureSound;

	void LoadShopItems();
	void ShopItemLoadFinished();
	void ShopWidgetGenerated(UUserWidget& NewWidget);

	UPROPERTY()
	TMap<const UPA_ShopItem*, const UShopItemWidget*> ItemsMap;

	UPROPERTY()
	UInventoryComponent* OwnerInventoryComponent;

	void ShowItemCombination(const UShopItemWidget* ItemWidget);

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PurchaseFailMessageText;

	FTimerHandle PurchaseFaileMessageTimerHandle;

	void HandlePurchaseFailed(EPurchaseResult Reason, const UPA_ShopItem* Item);
	void ClearPurchaseFailMessage();

	void HandlePurchaseSucceeded(const UPA_ShopItem* Item);
	void HandleItemSold(const UPA_ShopItem* Item);         
	void HandleSellFailed();                               
	void ShowMessage(const FText& Message);

	void UnbindInventoryEvents();

	UPROPERTY()
	class UAbilitySystemComponent* OwnerASC;

	bool bShopOpen = false;
	bool bItemStateDirty = false;
	bool bRefreshQueued = false;

	void HandleGoldChanged(const struct FOnAttributeChangeData& Data);
	void HandleInventoryItemAdded(const class UInventoryItem* Item);
	void HandleInventoryItemRemoved(const FInventoryItemHandle& Handle);
	void HandleInventoryStackCountChanged(const FInventoryItemHandle& Handle, int NewCount);

	void RequestItemStateRefresh();
	void RefreshAllItemStates();

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* CategoryButtonBox;

	UPROPERTY(EditDefaultsOnly, Category = "Category")
	TSubclassOf<class UShopCategoryButton> CategoryButtonClass;

	UPROPERTY(EditDefaultsOnly, Category = "Category")
	TArray<FShopCategoryInfo> Categories;

	UPROPERTY()
	TArray<class UShopCategoryButton*> CategoryButtons;

	UPROPERTY()
	TArray<const UPA_ShopItem*> AllShopItems;

	FGameplayTag CurrentCategory;

	void BuildCategoryButtons();
	void HandleCategoryClicked(const FGameplayTag& CategoryTag);
	void RefreshShopItemList();

	void UpdateCategoryButtonSelection();

	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* SearchTextBox;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* ClearSearchButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NoResultText;

	FString CurrentSearchQuery;

	FORCEINLINE bool IsSearching() const { return !CurrentSearchQuery.IsEmpty(); }

	UFUNCTION()
	void HandleSearchTextChanged(const FText& Text);

	UFUNCTION()
	void HandleSearchTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleClearSearchClicked();

	void ClearSearch();

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* UndoButton;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	class USoundBase* UndoSound;

	UFUNCTION()
	void HandleUndoClicked();

	void HandleUndoAvailabilityChanged(bool bCanUndo);
	void HandleUndoSucceeded();
	void HandleUndoFailed();
};
