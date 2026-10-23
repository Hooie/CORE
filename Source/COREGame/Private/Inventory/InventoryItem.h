// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "InventoryItem.generated.h"

class UPA_ShopItem;
class UAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAbilityCanCastUpdatedDelegate, bool /*bCanCast*/)

USTRUCT()
struct FInventoryItemHandle
{
	GENERATED_BODY()

public:

	FInventoryItemHandle();
	static FInventoryItemHandle InvalidHandle();
	static FInventoryItemHandle CreateHandle();

	bool IsValid() const;
	uint32 GetHandleId() const { return HandleId; }

private:
	explicit FInventoryItemHandle(uint32 Id);

	UPROPERTY()
	uint32 HandleId;

	static uint32 GenericNextId();
	static uint32 GetInvalidId();
};

bool operator==(const FInventoryItemHandle& Lhs, const FInventoryItemHandle& Rhs);
uint32 GetTypeHash(const FInventoryItemHandle& Key);

/**
 * 
 */
UCLASS()
class UInventoryItem : public UObject
{
	GENERATED_BODY()

public:
	UInventoryItem();

	FOnAbilityCanCastUpdatedDelegate OnAbilityCanCastUpdated;

	// 스택 카운트를 +1할 수 있다면 true 반환, Stackable 하지 않거나 최대 개수라면 false반환
	bool AddStackCount();
	// 스택 카운트를 -1한 뒤 0 이하라면 false 반환
	bool ReduceStackCount();

	// 스택 카운트를 설정할 수 있다면 true 반환
	bool SetStackCount(int NewStackCount);

	bool IsStackFull() const;
	bool IsForItem(const UPA_ShopItem* Item) const;

	bool IsGrantingAbility(TSubclassOf<class UGameplayAbility> AbilityClass) const;
	bool IsGrantingAnyAbility() const;

	bool IsValid() const;
	void InitItem(const FInventoryItemHandle& NewHandle, const UPA_ShopItem* NewShopItem, UAbilitySystemComponent* AbilitySystemComponent);
	const UPA_ShopItem* GetShopItem() const { return ShopItem; }
	FInventoryItemHandle GetHandle() const { return Handle; }

	bool TryActivateGrantedAbility();
	void ApplyConsumeEffect();
	void RemoveGASModifications();

	FORCEINLINE int GetStackCount() const { return StackCount; }
	void SetSlot(int NewSlot);
	int GetItemSlot() const { return Slot; }

	float GetAbilityCooldownTimeRemaining() const;
	float GetAbilityCooldownDuration() const;
	float GetAbilityManaCost() const;
	bool CanCastAbility() const;

	void SetGrantedAbilitySpecHandle(FGameplayAbilitySpecHandle SpecHandle) { GrantedAbilitySpecHandle = SpecHandle; }
	FGameplayAbilitySpecHandle GetGrantedAbilitySpecHandle() const { return GrantedAbilitySpecHandle; }

private:
	UAbilitySystemComponent* OwnerAbilitySystemComponent;

	void ManaUpdated(const FOnAttributeChangeData& ChangeData);

	void ApplyGASModifications();

	UPROPERTY()
	const UPA_ShopItem* ShopItem;
	
	FInventoryItemHandle Handle;

	int StackCount;
	int Slot;

	FActiveGameplayEffectHandle AppliedEquippedEffectHandle;
	TArray<FActiveGameplayEffectHandle> AppliedAdditionalEffectHandles;

	FGameplayAbilitySpecHandle GrantedAbilitySpecHandle;
};
