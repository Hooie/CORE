// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/ItemWidget.h"
#include "Inventory/InventoryItem.h"
#include "InventoryItemWidget.generated.h"

class UInventoryItem;
class UInventoryItemWidget;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemDropped, UInventoryItemWidget* /*Destination Widget*/, UInventoryItemWidget* /*Source Widget*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnButtonClicked, const FInventoryItemHandle& /*ItemHandle*/);

/**
 * 
 */
UCLASS()
class UInventoryItemWidget : public UItemWidget
{
	GENERATED_BODY()

public:
	FOnInventoryItemDropped OnInventoryItemDropped;
	FOnButtonClicked OnLeftButtonClicked;
	FOnButtonClicked OnRightButtonClicked;

	virtual void NativeConstruct() override;
	void UpdateInventoryItem(const UInventoryItem* Item);
	bool IsEmpty() const;
	void EmptySlot();
	FORCEINLINE int GetSlotNumber() const { return SlotNumber; }
	void SetSlotNumber(int NewSlotNumber);
	void UpdateStackCount();

	UTexture2D* GetIconTexture();

	FORCEINLINE const UInventoryItem* GetInventoryItem() const { return InventoryItem; }
	FInventoryItemHandle GetItemHandle() const;
	
	void SetHotkeyLabel(const FText& Label);

private:
	void UpdateCanCastDisplay(bool bCanCast);


	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	UTexture2D* EmptyTexture;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* StackCountText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CooldownCountText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CooldownDurationText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ManaCostText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HotkeyText;

	UPROPERTY()
	const UInventoryItem* InventoryItem;

	int SlotNumber;

	virtual void RightButtonClicked() override;
	virtual void LeftButtonClicked() override;

	/********************************************/
	/**************** Drag Drop *****************/
	/********************************************/
private:
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Drag Drop")
	TSubclassOf<class UInventoryItemDragDropOp> DragDropOpClass;

	/********************************************/
	/******************* GAS ********************/
	/********************************************/
public:
	void StartCooldown(float CooldownDuration, float TimeRemaining);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	float CooldownUpdateInterval = 0.1f;

	void BindCanCastAbilityDelegate();
	void UnBindCanCastAbilityDelegate();

	void CooldownFinished();
	void UpdateCooldown();
	void ClearCooldown();

	FTimerHandle CooldownDurationTimerHandle;
	FTimerHandle CooldownUpdateTimerHandle;

	float CooldownTimeRemaining = 0.f;
	float CooldownTimeDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FName CooldownAmtDynamicMaterialParamName = "Percent";

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FName IconTextureDynamicMaterialParamName = "Icon";

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FName CanCastDynamicMaterialParamName = "CanCast";

	virtual void SetIcon(UTexture2D* IconTexture) override;
	FNumberFormattingOptions CooldownDisplayFormattingOptions;
};
