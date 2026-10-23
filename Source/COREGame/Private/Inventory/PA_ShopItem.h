// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PA_ShopItem.generated.h"

class UGameplayEffect;
class UGameplayAbility;
class UPA_ShopItem;

USTRUCT(BlueprintType)
struct FItemCollection
{
	GENERATED_BODY()

public:
	FItemCollection();
	FItemCollection(const TArray<const UPA_ShopItem*>& InItems);
	
	void AddItem(const UPA_ShopItem* NewItem, bool bAddUnique = false);
	bool Contains(const UPA_ShopItem* Item) const;
	const TArray<const UPA_ShopItem*>& GetItems() const;

private:
	TArray<const UPA_ShopItem*> Items;
};

USTRUCT(BlueprintType)
struct FItemStatEntry
{
	GENERATED_BODY()

	FGameplayAttribute Attribute;
	float Magnitude = 0.f;
	bool bIsPercent = false;
};

/**
 * 
 */

UCLASS()
class UPA_ShopItem : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static FPrimaryAssetType GetShopItemAssetType();
	UTexture2D* GetIcon() const;
	const TSoftObjectPtr<UTexture2D>& GetIconSoftPtr() const { return Icon; }

	FText GetItemName() const { return ItemName; }
	FText GetItemDescription() const { return ItemDescription; }
	float GetPrice() const { return Price; }
	float GetSellPrice() const { return Price / 2.f; }
	bool GetIsConsumable() const { return bIsConsumable; }

	TSubclassOf<UGameplayEffect> GetEquippedEffect() const { return EquippedEffect; }
	const TArray<TSubclassOf<UGameplayEffect>>& GetAdditionalEquippedEffects() const { return AdditionalEquippedEffects; }
	TSubclassOf<UGameplayEffect> GetConsumeEffect() const { return ConsumeEffect; }
	TSubclassOf<UGameplayAbility> GetGrantedAbility() const { return GrantedAbility; }
	class UGameplayAbility* GetGrantedAbilityCDO() const;

	bool GetIsStackable() const { return bIsStackable; }
	bool GetIsUnique() const { return bIsUnique; }

	int GetMaxStackCount() const { return MaxStackCount; }
	const TArray<TSoftObjectPtr<UPA_ShopItem>>& GetIngredientItems() const { return IngredientItems; }

	float GetTotalIngredientCost() const;

	TArray<FItemStatEntry> GetStatEntries() const;

	const FGameplayTagContainer& GetItemCategories() const { return ItemCategories; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	float Price;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	FText ItemName;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	FText ItemDescription;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	bool bIsConsumable;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> EquippedEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<TSubclassOf<UGameplayEffect>> AdditionalEquippedEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ConsumeEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> GrantedAbility;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	bool bIsStackable = false;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	int MaxStackCount = 5;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	bool bIsUnique = false;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TArray<TSoftObjectPtr<UPA_ShopItem>> IngredientItems;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem", meta = (Categories = "ItemCategory"))
	FGameplayTagContainer ItemCategories;

};
