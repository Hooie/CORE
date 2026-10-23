// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/PA_ShopItem.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/CGameplayEffectSettings.h"

FPrimaryAssetId UPA_ShopItem::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetShopItemAssetType(), GetFName());
}

FPrimaryAssetType UPA_ShopItem::GetShopItemAssetType()
{
    return FPrimaryAssetType("ShopItem");
}

UTexture2D* UPA_ShopItem::GetIcon() const
{
    return Icon.LoadSynchronous();
}

UGameplayAbility* UPA_ShopItem::GetGrantedAbilityCDO() const
{
    if (GrantedAbility)
    {
        return Cast<UGameplayAbility>(GrantedAbility->GetDefaultObject());
    }
    
    return nullptr;
}

float UPA_ShopItem::GetTotalIngredientCost() const
{
	float Total = 0.f;

	for ( const TSoftObjectPtr<UPA_ShopItem>& IngredientPtr : IngredientItems )
	{
		if ( const UPA_ShopItem* Ingredient = IngredientPtr.LoadSynchronous() )
		{
			Total += Ingredient->GetPrice();
		}
	}

	return Total;
}

TArray<FItemStatEntry> UPA_ShopItem::GetStatEntries() const
{
	TArray<FItemStatEntry> Entries;

	if (!EquippedEffect)
		return Entries;

	const UGameplayEffect* EffectCDO = EquippedEffect->GetDefaultObject<UGameplayEffect>();
	if (!EffectCDO)
		return Entries;

	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();

	for ( const FGameplayModifierInfo& ModifierInfo : EffectCDO->Modifiers )
	{
		float Magnitude = 0.f;

		if ( !ModifierInfo.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Magnitude) )
			continue;

		if ( FMath::IsNearlyZero(Magnitude) )
			continue;

		FItemStatEntry Entry;
		Entry.Attribute = ModifierInfo.Attribute;

		if ( ModifierInfo.ModifierOp == EGameplayModOp::Multiplicitive )
		{
			Entry.bIsPercent = true;
			Entry.Magnitude = FMath::RoundToFloat((Magnitude - 1.f) * 100.f);
		}
		else if ( Settings && Settings->RatioAttributes.Contains(ModifierInfo.Attribute) )
		{
			Entry.bIsPercent = true;
			Entry.Magnitude = FMath::RoundToFloat(Magnitude * 100.f);
		}
		else
		{
			Entry.Magnitude = Magnitude;
		}

		Entries.Add(Entry);
	}

	return Entries;
}

FItemCollection::FItemCollection()
    :Items{}
{
}

FItemCollection::FItemCollection(const TArray<const UPA_ShopItem*>& InItems)
    :Items{InItems}
{
}

void FItemCollection::AddItem(const UPA_ShopItem* NewItem, bool bAddUnique)
{
    if (bAddUnique && Contains(NewItem))
        return;

    Items.Add(NewItem);
}

bool FItemCollection::Contains(const UPA_ShopItem* Item) const
{
    return Items.Contains(Item); 
}

const TArray<const UPA_ShopItem*>& FItemCollection::GetItems() const
{
    return Items;
}